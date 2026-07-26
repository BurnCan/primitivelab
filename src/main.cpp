// main.cpp — fullscreen + camera switching (FPS vs Editor) with position preservation
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <filesystem>
#include <memory>
#include "Primitives.h"
#include "Camera.h"
#include "SceneManager.h"
#include "ShaderUtils.h"
#include "AssetPaths.h"

namespace fs = std::filesystem;

// Window dimensions
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Shader directory
// Framebuffer (optional)
GLuint sceneFBO = 0;

// -----------------------------------------------------------------------------
// Camera & Mouse state
// -----------------------------------------------------------------------------
//static bool useFPSCamera = true;

//static bool firstMouse_FPS = true;
//static float lastX_FPS = SCR_WIDTH * 0.5f;
//static float lastY_FPS = SCR_HEIGHT * 0.5f;

//static bool firstMouse_Editor = true;
//static float lastX_Editor = SCR_WIDTH * 0.5f;
//static float lastY_Editor = SCR_HEIGHT * 0.5f;

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}



// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // --- Fullscreen Setup ---
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

    GLFWwindow* window = nullptr;
    if (monitor && mode) {
        SCR_WIDTH = mode->width;
        SCR_HEIGHT = mode->height;
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Game", monitor, nullptr);
        std::cout << "Fullscreen: " << SCR_WIDTH << "x" << SCR_HEIGHT << "\n";
    } else {
        std::cerr << "Falling back to windowed mode.\n";
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Game", nullptr, nullptr);
    }

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // Enable VSync to prevent unnecessarily high FPS such as 2500
    glfwSwapInterval(1); // Enable VSync 0=off 1=on
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // ---------------- Shader Programs ----------------
    //GLuint shaderProgram = CompileShaderProgram(SHADER_DIR + "basic.vert", SHADER_DIR + "basic.frag");
    //GLuint lightShaderProgram = CompileShaderProgram(SHADER_DIR + "light.vert", SHADER_DIR + "light.frag");
    Shader shader(ResolveAssetPath("basic.vert", "shaders").string(),
                  ResolveAssetPath("basic.frag", "shaders").string());
    Shader lightShader(ResolveAssetPath("light.vert", "shaders").string(),
                       ResolveAssetPath("light.frag", "shaders").string());

    // ---------------- SceneManager ----------------
    SceneManager scene;

    // ---------------- Cameras ----------------
    Camera editorCamera(glm::vec3(0.0f, 0.5f, 5.0f), glm::vec3(0,1,0), YAW, PITCH, true);
    FPSCamera fpsCamera(&scene, glm::vec3(0.0f,0.5f,5.0f), glm::vec3(0,1,0), YAW, PITCH, true);

    useFPSCamera = true;
    glfwSetWindowUserPointer(window, &fpsCamera);

    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---------------- Scene Load ----------------
    if (!scene.loadScene(ResolveAssetPath("default.txt", "scenes").string())) {
        std::cout << "No scene found, creating default one...\n";
        scene.AddPrimitive(std::make_unique<Primitive>(PrimitiveType::Cube, "textures/brick.jpg"));
        scene.AddPrimitive(std::make_unique<Primitive>(PrimitiveType::TriangularPrism, "textures/wood.jpg"));
        scene.AddPrimitive(std::make_unique<Primitive>(PrimitiveType::Sphere, "textures/earth.jpg",16,16));

        Light light;
        light.position = glm::vec3(-2.0f,1.0f,-2.0f);
        light.color = glm::vec3(1.0f);
        scene.AddLight(light);
    }

    // Scene and shader loading can take long enough for the launching window to
    // retain focus. Explicitly focus the game once startup is complete so key
    // input (including WASD) works without requiring an initial mouse click.
    glfwFocusWindow(window);

    // ---------------- Main Loop ----------------
    static bool tabWasDown = false;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // ---------------- Camera Toggle ----------------
        bool tabDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabDown && !tabWasDown) {
            useFPSCamera = !useFPSCamera;

            if (useFPSCamera) {
                fpsCamera.Position = editorCamera.Position;
                fpsCamera.Yaw = editorCamera.Yaw;
                fpsCamera.Pitch = editorCamera.Pitch;
                fpsCamera.updateCameraVectors();
                glfwSetWindowUserPointer(window, &fpsCamera);
                firstMouse_FPS = true;
                std::cout << "Switched to FPS Camera\n";
            } else {
                editorCamera.Position = fpsCamera.Position;
                editorCamera.Yaw = fpsCamera.Yaw;
                editorCamera.Pitch = fpsCamera.Pitch;
                editorCamera.updateCameraVectors();
                glfwSetWindowUserPointer(window, &editorCamera);
                firstMouse_Editor = true;
                std::cout << "Switched to Editor Camera\n";
            }
        }
        tabWasDown = tabDown;

        // Process camera input
        if (useFPSCamera)
            fpsCamera.ProcessInput(window, deltaTime);
        else
            editorCamera.ProcessInput(window, deltaTime);

        // --- Render ---
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (useFPSCamera) {
            scene.drawScene(shader, fpsCamera, SCR_WIDTH, SCR_HEIGHT);
            scene.drawLights(lightShader, fpsCamera, SCR_WIDTH, SCR_HEIGHT);
        } else {
            scene.drawScene(shader, editorCamera, SCR_WIDTH, SCR_HEIGHT);
            scene.drawLights(lightShader, editorCamera, SCR_WIDTH, SCR_HEIGHT);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
