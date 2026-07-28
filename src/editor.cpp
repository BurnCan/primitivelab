#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>

#include "Camera.h"
#include "SceneManager.h"
#include "ShaderUtils.h"
#include "AssetPaths.h"
#include "KeyboardInput.h"

// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <filesystem>
#include "external/tinyfiledialogs.h"

// Window size
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Shader directory
const std::string SHADER_DIR = ResolveAssetPath("basic.vert", "shaders").parent_path().string() + "/";



// Camera defaults
#define YAW   -90.0f
#define PITCH   0.0f

//Bounding box
static bool showCollisionMeshes = false;
static bool showTerrainMesh = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

//static bool useFPSCamera = false;

static void EditorMouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        return;

    if (useFPSCamera) {
        FPSCamera* cam = static_cast<FPSCamera*>(glfwGetWindowUserPointer(window));
        if (cam) cam->MouseCallback(window, xpos, ypos);
    } else {
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        static bool firstMouse = true;
        static float lastX = 0.0f, lastY = 0.0f;

        if (firstMouse) {
            lastX = (float)xpos;
            lastY = (float)ypos;
            firstMouse = false;
        }

        float xoffset = (float)xpos - lastX;
        float yoffset = lastY - (float)ypos; // reversed: y goes up
        lastX = (float)xpos;
        lastY = (float)ypos;

        if (cam)
            cam->ProcessMouseMovement(xoffset, yoffset);
    }
}




// Shader utilities (same as before)
std::string LoadShaderSource(const std::string& path);
GLuint CompileShader(GLenum type, const std::string& source);
GLuint CompileShaderProgram(const std::string& vertPath, const std::string& fragPath);

int main() {
    // ---------------- GLFW ----------------
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Editor", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    // Enable VSync to prevent unnecessarily high FPS such as 2500
    glfwSwapInterval(1); // Enable VSync 0=off 1=on
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ---------------- GLAD ----------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);










    // ---------------- Shaders ----------------
    //GLuint shaderProgram      = CompileShaderProgram(SHADER_DIR + "basic.vert", SHADER_DIR + "basic.frag");
    //GLuint lightShaderProgram = CompileShaderProgram(SHADER_DIR + "light.vert", SHADER_DIR + "light.frag");
    Shader shader(SHADER_DIR + "basic.vert", SHADER_DIR + "basic.frag");
    Shader lightShader(SHADER_DIR + "light.vert", SHADER_DIR + "light.frag");
    Shader boundingBoxShader(SHADER_DIR + "boundingBox.vert", SHADER_DIR + "boundingBox.frag");
    // ---------------- SceneManager ----------------
SceneManager sceneManager;
std::string currentSceneFile; // track currently loaded/saved scene path

std::string defaultScenePath = ResolveAssetPath("default.txt", "scenes").string();
if (sceneManager.loadScene(defaultScenePath)) {
    std::cout << "Loaded default scene: " << defaultScenePath << std::endl;
    currentSceneFile = defaultScenePath;
} else {
    std::cout << "No scene found, creating default one...\n";

    auto cube   = std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::Cube, "textures/brick.jpg");
    auto prism  = std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::TriangularPrism, "textures/wood.jpg");
    auto sphere = std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::Sphere, "textures/earth.jpg", 16, 16);

    sceneManager.AddSceneObject(std::move(cube));
    sceneManager.AddSceneObject(std::move(prism));
    sceneManager.AddSceneObject(std::move(sphere));

    Light light;
    light.position = glm::vec3(-2.0f,1.0f,-2.0f);
    light.color    = glm::vec3(1.0f);
    sceneManager.AddLight(light);

    // Track it even if just created
    currentSceneFile = defaultScenePath;
}

// ---------------- Camera ----------------

    Camera editorCamera(glm::vec3(0,0.5,5), glm::vec3(0,1,0), YAW, PITCH, true);

    FPSCamera fpsCamera(&sceneManager, glm::vec3(0.0f, 1.5f, 5.0f), glm::vec3(0, 1, 0), -90.0f, 0.0f, true);

    // Assign default pointer and callbacks for GLFW
if (useFPSCamera) {
    glfwSetWindowUserPointer(window, &fpsCamera);
    glfwSetCursorPosCallback(window, MouseCallback);  // FPS version
} else {
    glfwSetWindowUserPointer(window, &editorCamera);
    glfwSetCursorPosCallback(window, EditorMouseCallback);  // Editor version
}

glfwSetScrollCallback(window, ScrollCallback);
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



    // ---------------- ImGui ----------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.FontGlobalScale = 2.0f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // ---------------- Framebuffer ----------------
    GLuint sceneFBO, sceneTexture, rbo;
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---------------- Main Loop ----------------
    const int numWindows = 2;
    int focusedWindowIndex = 1;
    bool f1PressedLast = false;

    while(!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Switch windows
        bool f1Pressed = IsKeyPressed(window, GLFW_KEY_F1);
        if (f1Pressed && !f1PressedLast) focusedWindowIndex = (focusedWindowIndex + 1) % numWindows;
        f1PressedLast = f1Pressed;

        // ---------------- ImGui Frame ----------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        //std::string currentSceneFile; // track currently open scene
      if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
    // Clear current scene
    sceneManager.Clear();
    currentSceneFile.clear();

    // Prompt for a filename
    const char* filters[] = { "*.txt" };
    const char* path = tinyfd_saveFileDialog(
        "Create New Scene",
        "../scenes/new_scene.txt", // suggested default
        1, filters, "Scene files"
    );

    if (path) {
        currentSceneFile = path;

        // Immediately save empty scene so Save works right away
        if (sceneManager.saveScene(currentSceneFile)) {
            std::cout << "Created new scene: " << currentSceneFile << std::endl;
        }
    } else {
        std::cout << "New Scene canceled." << std::endl;
    }
}


        if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
            const char* path = tinyfd_openFileDialog(
                "Open Scene",
                "../scenes",
                0,       // number of filters
                nullptr, // filter patterns
                nullptr, // description
                0        // single file only
            );
            if (path) {
                if (sceneManager.loadScene(path)) {
                    currentSceneFile = path;
                    std::cout << "Opened scene: " << path << std::endl;
                } else {
                    std::cerr << "Failed to open scene: " << path << std::endl;
                }
            }
        }

        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            if (!currentSceneFile.empty()) {
                if (sceneManager.saveScene(currentSceneFile))
                    std::cout << "Saved scene: " << currentSceneFile << std::endl;
                else
                    std::cerr << "Failed to save scene: " << currentSceneFile << std::endl;
            } else {
                std::cout << "No current scene file. Use Save As...\n";
            }
        }

        if (ImGui::MenuItem("Save Scene As...")) {
            const char* path = tinyfd_saveFileDialog(
                "Save Scene As",
                "../scenes/untitled.txt",
                0,
                nullptr,
                nullptr
            );
            if (path) {
                if (sceneManager.saveScene(path)) {
                    currentSceneFile = path;
                    std::cout << "Scene saved as: " << path << std::endl;
                } else {
                    std::cerr << "Failed to save scene as: " << path << std::endl;
                }
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Camera")) {
    if (ImGui::MenuItem("FPS Camera", nullptr, useFPSCamera)) {
        if (!useFPSCamera) {
            // Preserve camera state from editor -> FPS
            fpsCamera.Position = editorCamera.Position;
            fpsCamera.Yaw = editorCamera.Yaw;
            fpsCamera.Pitch = editorCamera.Pitch;
            fpsCamera.updateCameraVectors();

            useFPSCamera = true;
            glfwSetWindowUserPointer(window, &fpsCamera);
            firstMouse_FPS = true;


            std::cout << "[Camera] Switched to FPS Camera\n";
        }
    }

    if (ImGui::MenuItem("Editor Camera", nullptr, !useFPSCamera)) {
        if (useFPSCamera) {
            // Preserve camera state from FPS -> editor
            editorCamera.Position = fpsCamera.Position;
            editorCamera.Yaw = fpsCamera.Yaw;
            editorCamera.Pitch = fpsCamera.Pitch;
            editorCamera.updateCameraVectors();

            useFPSCamera = false;
            glfwSetWindowUserPointer(window, &editorCamera);
            firstMouse_Editor = true;

            std::cout << "[Camera] Switched to Editor Camera\n";
        }
    }
    ImGui::EndMenu();
}

    ImGui::EndMainMenuBar();
}







        // Dockspace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGuiWindowFlags dockFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("DockSpace", nullptr, dockFlags);
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0,0));
        ImGui::End();

        // ---------------- Scene Window ----------------
        ImGui::Begin("Scene");
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if ((int)viewportSize.x > 0 && (int)viewportSize.y > 0 &&
            ((int)viewportSize.x != SCR_WIDTH || (int)viewportSize.y != SCR_HEIGHT)) {
            SCR_WIDTH = (int)viewportSize.x;
            SCR_HEIGHT = (int)viewportSize.y;
            glBindTexture(GL_TEXTURE_2D, sceneTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
        }

        bool sceneFocused = (focusedWindowIndex == 0);
if (sceneFocused) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (useFPSCamera)
        fpsCamera.ProcessInput(window, deltaTime);
    else
        editorCamera.ProcessInput(window, deltaTime);
} else {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}




        // --- Render Scene ---
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw all scene objects via SceneManager.
        sceneManager.drawScene(shader,
                       useFPSCamera ? fpsCamera : editorCamera,
                       SCR_WIDTH,
                       SCR_HEIGHT);

        // Draw light spheres
        sceneManager.drawLights(lightShader,
                       useFPSCamera ? fpsCamera : editorCamera,
                       SCR_WIDTH,
                       SCR_HEIGHT);

        // Draw bounding boxes (optional debug overlay)
        if (showCollisionMeshes || showTerrainMesh) {
            sceneManager.DrawCollisionMeshes(boundingBoxShader,
                useFPSCamera ? fpsCamera : editorCamera,
                SCR_WIDTH, SCR_HEIGHT, showCollisionMeshes, showTerrainMesh);
        }




        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ImGui::Image((void*)(intptr_t)sceneTexture, viewportSize, ImVec2(0,1), ImVec2(1,0));
        ImGui::End();

        // ---------------- Light Editor ----------------
        ImGui::Begin("Light Editor");

        ImVec4 titleColor = (focusedWindowIndex == 1) ? ImVec4(0.9f,0.7f,0.2f,1.0f) : ImVec4(1,1,1,1);
        ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
        ImGui::Text("Light Editor Controls");
        ImGui::PopStyleColor();

        // Iterate over lights in the scene manager
        auto& lights = sceneManager.GetLights();
        for (size_t i = 0; i < lights.size(); ++i) {
            Light& light = lights[i];

            ImGui::Separator();
            ImGui::Text("Light %d", (int)i);

            // Position slider
            ImGui::SliderFloat3(("Position##" + std::to_string(i)).c_str(), &light.position[0], -10.0f, 10.0f);

            // Color editor
            ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), &light.color[0]);

            // Optional per-light shininess slider
            // ImGui::SliderFloat(("Shininess##" + std::to_string(i)).c_str(), &light.shininess, 1.0f, 128.0f);
        }

        // Button to add a new light
        if (ImGui::Button("Add Light")) {
            Light newLight;
            newLight.position = glm::vec3(0.0f, 1.0f, 0.0f);
            newLight.color    = glm::vec3(1.0f);
            sceneManager.AddLight(newLight);
        }

        // Button to remove the last light
        if (ImGui::Button("Remove Last Light") && !sceneManager.GetLights().empty()) {
            sceneManager.GetLights().pop_back();
        }

        ImGui::End();

// === Scene Object Editor Window ===
ImGui::Begin("Scene Objects");

const std::filesystem::path textureDirectory = ResolveAssetPath("textures", "textures");
std::vector<std::string> textureFiles = GetAvailableTextures(textureDirectory.string());
static int newPrimitiveTextureIndex = 0;
const bool hasTextures = !textureFiles.empty();
if (hasTextures && newPrimitiveTextureIndex >= static_cast<int>(textureFiles.size())) newPrimitiveTextureIndex = 0;
const char* texturePreview = hasTextures ? textureFiles[newPrimitiveTextureIndex].c_str() : "No textures found";
if (ImGui::BeginCombo("Texture##NewSceneObject", texturePreview)) {
    for (int i = 0; i < static_cast<int>(textureFiles.size()); ++i) {
        const bool selected = newPrimitiveTextureIndex == i;
        if (ImGui::Selectable(textureFiles[i].c_str(), selected)) newPrimitiveTextureIndex = i;
        if (selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
}
const std::string creationTexture = hasTextures ? textureFiles[newPrimitiveTextureIndex] : std::string{};
if (!hasTextures) ImGui::BeginDisabled();
ImGui::Text("Add 2D Primitive");
if (ImGui::Button("Triangle")) sceneManager.AddSceneObject(std::make_unique<TwoD::Primitive>(TwoD::PrimitiveType::Triangle, creationTexture));
ImGui::SameLine();
if (ImGui::Button("Plane")) sceneManager.AddSceneObject(std::make_unique<TwoD::Primitive>(TwoD::PrimitiveType::Plane, creationTexture));
ImGui::Text("Add 3D Primitive");
if (ImGui::Button("Cube")) sceneManager.AddSceneObject(std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::Cube, creationTexture));
ImGui::SameLine();
if (ImGui::Button("Triangular Prism")) sceneManager.AddSceneObject(std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::TriangularPrism, creationTexture));
ImGui::SameLine();
if (ImGui::Button("Sphere")) sceneManager.AddSceneObject(std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::Sphere, creationTexture));
ImGui::SameLine();
if (ImGui::Button("Slab")) sceneManager.AddSceneObject(std::make_unique<ThreeD::Primitive>(ThreeD::PrimitiveType::Slab, creationTexture));
if (!hasTextures) ImGui::EndDisabled();
ImGui::Separator();

const auto& sceneObjects = sceneManager.GetSceneObjects();
int objectToDelete = -1;
for (int i = 0; i < static_cast<int>(sceneObjects.size()); ++i) {
    SceneObject* object = sceneObjects[i].get();
    auto* prim = dynamic_cast<detail::PrimitiveMesh*>(object);
    if (!object || !prim) continue;
    const char* category = object->GetDimension() == SceneDimension::TwoD ? "2D Primitive" : "3D Primitive";
    ImGui::PushID(i);
    const std::string heading = object->GetName() + " — " + category + "##" + std::to_string(i);
    if (ImGui::CollapsingHeader(heading.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, ("SceneObjectColumns##" + std::to_string(i)).c_str(), false);
        ImGui::Text("Transform (3D world placement)");
        if (ImGui::SliderFloat3("Position", &prim->position[0], -10.0f, 10.0f)) prim->UpdateModelMatrix();
        if (ImGui::SliderFloat3("Rotation", &prim->rotation[0], -180.0f, 180.0f)) prim->UpdateModelMatrix();
        if (ImGui::SliderFloat3("Scale", &prim->scale[0], 0.1f, 5.0f)) prim->UpdateModelMatrix();
        ImGui::NextColumn();
        ImGui::Text("Texture");
        const std::string currentTex = prim->GetTexturePath().empty() ? "(none)" : prim->GetTexturePath();
        if (ImGui::BeginCombo("Select Texture", currentTex.c_str())) {
            for (const auto& tex : textureFiles) {
                const bool selected = tex == prim->GetTexturePath();
                if (ImGui::Selectable(tex.c_str(), selected)) prim->SetTexturePath((std::filesystem::path("textures") / tex).generic_string());
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Image((void*)(intptr_t)prim->GetTextureID(), ImVec2(64, 64), ImVec2(0,1), ImVec2(1,0));
        if (ImGui::Button("Delete")) objectToDelete = i;
        ImGui::Columns(1);
        ImGui::Separator();
    }
    ImGui::PopID();
}
if (objectToDelete >= 0) sceneManager.RemoveSceneObject(static_cast<std::size_t>(objectToDelete));
ImGui::End();


// ---------------- Debug Window ----------------


ImGui::Begin("Debug");

ImGui::Text("Diagnostics");
ImGui::Separator();

// FPS counter
ImGui::Text("Frame Time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

// Bounding box toggle
ImGui::Checkbox("Show Collision Meshes", &showCollisionMeshes);
ImGui::Checkbox("Show Terrain Mesh", &showTerrainMesh);

// Optional: Add wireframe toggle
static bool wireframeMode = false;
if (ImGui::Checkbox("Wireframe Mode", &wireframeMode)) {
    glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
}

ImGui::End();







        // ---------------- Render ImGui ----------------
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ---------------- Cleanup ----------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
