#include "Camera.h"
#include "SceneManager.h"
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
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
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
    float velocity = MovementSpeed * deltaTime;
    glm::vec3 proposed = Position;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        proposed += Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        proposed -= Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        proposed -= Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        proposed += Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        proposed.y += velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        proposed.y -= velocity;

    // The collision volume must enclose the camera's near clipping plane, not
    // merely the view position, or surfaces can be clipped before the point at
    // the center of the camera registers a collision.
    if (!scene || !scene->CheckCollision(proposed, FPS_CAMERA_COLLISION_RADIUS)) {
        Position = proposed;
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
