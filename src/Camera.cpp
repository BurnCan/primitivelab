#include "Camera.h"
#include "SceneManager.h"
#include <algorithm>
#include <iostream>

// -----------------------------------------------------------------------------
// Shared camera & mouse state
// -----------------------------------------------------------------------------
bool useFPSCamera = true;

bool firstMouse_FPS = true;
float lastX_FPS = 0.0f;
float lastY_FPS = 0.0f;

bool firstMouse_Editor = true;
float lastX_Editor = 0.0f;
float lastY_Editor = 0.0f;

// -----------------------------------------------------------------------------
// Mouse Input Callback
// -----------------------------------------------------------------------------
void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        return;

    if (useFPSCamera)
    {
        FPSCamera* cam = static_cast<FPSCamera*>(glfwGetWindowUserPointer(window));
        if (!cam) return;

        if (firstMouse_FPS)
        {
            lastX_FPS = (float)xpos;
            lastY_FPS = (float)ypos;
            firstMouse_FPS = false;
        }

        float xoffset = (float)xpos - lastX_FPS;
        float yoffset = lastY_FPS - (float)ypos;

        lastX_FPS = (float)xpos;
        lastY_FPS = (float)ypos;

        cam->ProcessMouseMovement(xoffset, yoffset);
    }
    else
    {
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (!cam) return;

        if (firstMouse_Editor)
        {
            lastX_Editor = (float)xpos;
            lastY_Editor = (float)ypos;
            firstMouse_Editor = false;
        }

        float xoffset = (float)xpos - lastX_Editor;
        float yoffset = lastY_Editor - (float)ypos;

        lastX_Editor = (float)xpos;
        lastY_Editor = (float)ypos;

        cam->ProcessMouseMovement(xoffset, yoffset);
    }
}

// -----------------------------------------------------------------------------
// Scroll Input Callback
// -----------------------------------------------------------------------------
void ScrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    void* ptr = glfwGetWindowUserPointer(window);
    if (!ptr) return;

    if (useFPSCamera)
    {
        FPSCamera* cam = static_cast<FPSCamera*>(ptr);
        if (cam) cam->ProcessMouseScroll((float)yoffset);
    }
    else
    {
        Camera* cam = static_cast<Camera*>(ptr);
        if (cam) cam->ProcessMouseScroll((float)yoffset);
    }
}


// =============================
//        CAMERA BASE
// =============================
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch, bool invertY)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY),
      Zoom(ZOOM),
      InvertY(invertY)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

// ✅ NEW: Projection Matrix
glm::mat4 Camera::GetProjectionMatrix(int width, int height) const {
    // A minimized window (or an empty editor viewport) can temporarily report
    // zero dimensions. GLM requires a non-zero aspect ratio.
    const int safeWidth = std::max(width, 1);
    const int safeHeight = std::max(height, 1);
    float aspectRatio = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;

    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessInput(GLFWwindow* window, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Position += Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Position -= Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Position -= Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Position += Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        Position.y += velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        Position.y -= velocity;
}




void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    if (Zoom < 1.0f) Zoom = 1.0f;
    if (Zoom > 45.0f) Zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}

// =============================
//        FPS CAMERA
// =============================
FPSCamera::FPSCamera(SceneManager* sceneManager,
                     glm::vec3 position,
                     glm::vec3 up,
                     float yaw,
                     float pitch,
                     bool invertY)
    : Camera(position, up, yaw, pitch, invertY),
      scene(sceneManager)
{
}

// --- Collision-aware keyboard ---
void FPSCamera::ProcessInput(GLFWwindow* window, float deltaTime) {
    // Avoid an unusually long frame turning into a large physics step.
    const float frameTime = std::min(deltaTime, 0.05f);
    const float velocity = MovementSpeed * frameTime;
    glm::vec3 movement(0.0f);

    // FPS movement stays parallel to the ground even while looking up/down.
    glm::vec3 forward(Front.x, 0.0f, Front.z);
    if (glm::dot(forward, forward) > 0.0001f)
        forward = glm::normalize(forward);
    glm::vec3 right(Right.x, 0.0f, Right.z);
    if (glm::dot(right, right) > 0.0001f)
        right = glm::normalize(right);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += right;
    if (glm::dot(movement, movement) > 1.0f)
        movement = glm::normalize(movement);
    movement *= velocity;

    const bool crouchPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    // Keep the bottom of the collision volume fixed while changing height.
    // Standing is refused when there is not enough headroom.
    if (crouchPressed && !IsCrouching) {
        Position.y -= FPS_CAMERA_COLLISION_RADIUS - FPS_CAMERA_CROUCH_RADIUS;
        CollisionRadius = FPS_CAMERA_CROUCH_RADIUS;
        IsCrouching = true;
    } else if (!crouchPressed && IsCrouching) {
        const float heightDifference = FPS_CAMERA_COLLISION_RADIUS - FPS_CAMERA_CROUCH_RADIUS;
        const glm::vec3 standingPosition = Position + glm::vec3(0.0f, heightDifference, 0.0f);
        if (!scene || !scene->CheckCollision(standingPosition, FPS_CAMERA_COLLISION_RADIUS)) {
            Position = standingPosition;
            CollisionRadius = FPS_CAMERA_COLLISION_RADIUS;
            IsCrouching = false;
        }
    }

    // A short downward probe makes grounding stable when the sphere is resting
    // exactly against a floor (which is not itself an intersection).
    constexpr float groundProbeDistance = 0.03f;
    glm::vec3 groundNormal(0.0f);
    IsGrounded = scene && scene->CheckCollision(
        Position - glm::vec3(0.0f, groundProbeDistance, 0.0f),
        CollisionRadius, &groundNormal) && groundNormal.y > 0.5f;

    const bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jumpPressed && !jumpWasPressed && IsGrounded) {
        VerticalVelocity = FPS_JUMP_SPEED;
        IsGrounded = false;
    }
    jumpWasPressed = jumpPressed;

    if (!IsGrounded)
        VerticalVelocity -= FPS_GRAVITY * frameTime;
    else if (VerticalVelocity < 0.0f)
        VerticalVelocity = 0.0f;

    movement.y = VerticalVelocity * frameTime;

    // The collision volume must enclose the camera's near clipping plane, not
    // merely the view position, or surfaces can be clipped before the point at
    // the center of the camera registers a collision.
    if (!scene) {
        Position += movement;
        return;
    }

    // Move up to the point of impact before removing the part of the remaining
    // movement that points into the surface. Projecting the complete movement
    // from the original position leaves its end point inside the collision
    // volume. That is particularly noticeable at a convex edge: each adjacent
    // face can reject the projected end point, making the camera stop instead
    // of travelling around the edge.
    glm::vec3 remaining = movement;
    constexpr int maxSlideIterations = 5;
    constexpr int sweepIterations = 12;
    constexpr float minimumMovementSquared = 0.0000001f;
    for (int iteration = 0;
         iteration < maxSlideIterations && glm::dot(remaining, remaining) > minimumMovementSquared;
         ++iteration) {
        const glm::vec3 proposed = Position + remaining;
        glm::vec3 contactNormal(0.0f);
        if (!scene->CheckCollision(proposed, CollisionRadius, &contactNormal)) {
            Position = proposed;
            break;
        }

        // Find the last collision-free point on this movement segment so the
        // tangent movement begins at a stable contact point.
        float safeFraction = 0.0f;
        float collidingFraction = 1.0f;
        for (int sweep = 0; sweep < sweepIterations; ++sweep) {
            const float testFraction = (safeFraction + collidingFraction) * 0.5f;
            glm::vec3 testNormal(0.0f);
            if (scene->CheckCollision(Position + remaining * testFraction,
                                      CollisionRadius, &testNormal)) {
                collidingFraction = testFraction;
                contactNormal = testNormal;
            } else {
                safeFraction = testFraction;
            }
        }

        Position += remaining * safeFraction;
        remaining *= 1.0f - safeFraction;

        const float intoSurface = glm::dot(remaining, contactNormal);
        if (contactNormal.y > 0.5f && VerticalVelocity < 0.0f) {
            IsGrounded = true;
            VerticalVelocity = 0.0f;
        } else if (contactNormal.y < -0.5f && VerticalVelocity > 0.0f) {
            VerticalVelocity = 0.0f;
        }
        if (intoSurface >= 0.0f || glm::dot(contactNormal, contactNormal) < 0.5f)
            break;

        remaining -= contactNormal * intoSurface;
    }
}

// --- Static mouse callback for GLFW ---
void FPSCamera::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    FPSCamera* cam = static_cast<FPSCamera*>(glfwGetWindowUserPointer(window));
    if (!cam) return;

    if (cam->firstMouse) {
        cam->lastX = (float)xpos;
        cam->lastY = (float)ypos;
        cam->firstMouse = false;
    }

    float xoffset = (float)xpos - cam->lastX;
    float yoffset = cam->lastY - (float)ypos; // reversed: y-coord ranges bottom->top

    cam->lastX = (float)xpos;
    cam->lastY = (float)ypos;

    cam->ProcessMouseMovement(xoffset, yoffset);
}
