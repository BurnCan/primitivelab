#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declaration to avoid circular include
class SceneManager;

// Default camera values
const float YAW         = -90.0f;
const float PITCH       = 0.0f;
const float SPEED       = 1.0f;
const float SENSITIVITY = 0.25f;
const float ZOOM        = 45.0f;

// Keep the FPS camera farther from rendered surfaces than the projection near
// plane. This prevents the near plane (including its corners) from passing
// through a primitive while the camera's collision volume is still clear.
const float FPS_CAMERA_COLLISION_RADIUS = 0.25f;
const float FPS_CAMERA_CROUCH_RADIUS = 0.15f;
const float FPS_GRAVITY = 9.81f;
const float FPS_JUMP_SPEED = 2.75f;

// =============================
//        BASE CAMERA CLASS
// =============================
class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    bool InvertY;



    Camera(glm::vec3 position = glm::vec3(0.0f, 0.5f, 5.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH,
           bool invertY = false);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(int width, int height) const;

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);
    virtual void ProcessInput(GLFWwindow* window, float deltaTime);

    void updateCameraVectors();



protected:

};

// =============================
//        FPS CAMERA CLASS
// =============================
class FPSCamera : public Camera {
public:
    SceneManager* scene;  // Reference to scene for collision detection

    // Mouse state
    bool firstMouse = true;
    float lastX = 0.0f;
    float lastY = 0.0f;

    // Vertical motion belongs to the FPS camera rather than the free-flying
    // editor camera. The collision radius also doubles as the camera's height
    // above the floor, so reducing it produces a compact crouch.
    float VerticalVelocity = 0.0f;
    float CollisionRadius = FPS_CAMERA_COLLISION_RADIUS;
    bool IsGrounded = false;
    bool IsCrouching = false;
    bool jumpWasPressed = false;

    FPSCamera(SceneManager* sceneManager,
              glm::vec3 position = glm::vec3(0.0f, 0.5f, 5.0f),
              glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
              float yaw = YAW,
              float pitch = PITCH,
              bool invertY = false);

    //void ProcessKeyboard(char direction, float deltaTime);
    void ProcessInput(GLFWwindow* window, float deltaTime) override;

    // Static mouse callback for GLFW
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
};

// Shared camera state
extern bool useFPSCamera;

extern bool firstMouse_FPS;
extern float lastX_FPS;
extern float lastY_FPS;

extern bool firstMouse_Editor;
extern float lastX_Editor;
extern float lastY_Editor;

// Callback declarations
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
