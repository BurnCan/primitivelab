#include <glad/glad.h>
#include "TextureUtils.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <optional>
#include <algorithm>
#include <cstdio>
#include <type_traits>
#include <vector>

#include "Camera.h"
#include "SceneManager.h"
#include "ShaderUtils.h"
#include "AssetPaths.h"
#include "SkyboxAssets.h"
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
const std::string SHADER_DIR = ResolveAssetPath("texture/basic.vert", "shaders").parent_path().parent_path().string() + "/";



// Camera defaults
#define YAW   -90.0f
#define PITCH   0.0f

//Bounding box
static bool showCollisionMeshes = false;
static bool showTerrainMesh = false;

enum class CreationOption {
    Triangle,
    Plane,
    Cube,
    TriangularPrism,
    Sphere,
    Slab,
    Terrain,
    Skybox,
    Rock,
    Light
};

struct CreationOptionOption {
    CreationOption type;
    const char* name;
};

static bool CreationOptionGroup(const char* label, const CreationOptionOption* options,
                                 int optionCount, CreationOption& selectedType) {
    bool selectionChanged = false;
    ImGui::TextDisabled("%s", label);
    ImGui::Indent();
    for (int index = 0; index < optionCount; ++index) {
        ImGui::PushID(static_cast<int>(options[index].type));
        const bool isSelected = selectedType == options[index].type;
        if (ImGui::Selectable(options[index].name, isSelected, 0, ImVec2(0.0f, 0.0f))) {
            selectedType = options[index].type;
            selectionChanged = true;
        }
        if (isSelected) ImGui::SetItemDefaultFocus();
        ImGui::PopID();
    }
    ImGui::Unindent();
    return selectionChanged;
}

static bool DragVector3(const char* label, float* values, float speed,
                        float minimum = 0.0f, float maximum = 0.0f) {
    ImGui::TextUnformatted(label);
    ImGui::PushID(label);
    bool changed = false;
    if (ImGui::BeginTable("##Components", 3, ImGuiTableFlags_SizingStretchSame)) {
        static const char* componentLabels[] = {"X", "Y", "Z"};
        for (int component = 0; component < 3; ++component) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(componentLabels[component]);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::PushID(component);
            changed |= ImGui::DragFloat("##Value", &values[component], speed, minimum, maximum);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
    return changed;
}

template <typename SetTexture>
static void TextureSelector(const std::vector<std::string>& textureFiles,
                            const std::string& currentTexture, SetTexture&& setTexture) {
    const std::string currentFilename = std::filesystem::path(currentTexture).filename().string();
    const char* preview = currentFilename.empty() ? "No texture" : currentFilename.c_str();

    ImGui::TextUnformatted("Texture");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##Texture", preview)) {
        if (textureFiles.empty()) {
            ImGui::TextDisabled("No textures found");
        } else {
            for (const std::string& texture : textureFiles) {
                const bool selected = texture == currentFilename;
                if (ImGui::Selectable(texture.c_str(), selected)) setTexture(texture);
                if (selected) ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

static void SkyboxTextureSelector(const std::vector<std::string>& skyboxNames, Skybox& skybox) {
    const std::string& current = skybox.GetConfig().name;
    const char* preview = current.empty() ? "No skybox selected" : current.c_str();

    ImGui::TextUnformatted("Texture");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##Texture", preview)) {
        if (skyboxNames.empty()) ImGui::TextDisabled("No valid skyboxes found");
        for (const auto& name : skyboxNames) {
            const bool selected = name == current;
            if (ImGui::Selectable(name.c_str(), selected) && skybox.Reload({name}))
                std::cout << "[Skybox] Loaded '" << name << "'\n";
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

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
    Shader shader(SHADER_DIR + "texture/basic.vert", SHADER_DIR + "texture/basic.frag");
    Shader lightShader(SHADER_DIR + "engine/light.vert", SHADER_DIR + "engine/light.frag");
    Shader boundingBoxShader(SHADER_DIR + "engine/boundingBox.vert", SHADER_DIR + "engine/boundingBox.frag");
    Shader skyboxShader(SHADER_DIR + "engine/skybox.vert", SHADER_DIR + "engine/skybox.frag");
    MaterialRegistry materials(SHADER_DIR);
    // ---------------- SceneManager ----------------
SceneManager sceneManager;
std::vector<std::string> skyboxNames = SkyboxAssets::Discover();
std::string currentSceneFile; // track currently loaded/saved scene path

std::string defaultScenePath = ResolveAssetPath("default.txt", "scenes").string();
if (sceneManager.loadScene(defaultScenePath)) {
    std::cout << "Loaded default scene: " << defaultScenePath << std::endl;
    currentSceneFile = defaultScenePath;
} else {
    std::cout << "No scene found, creating default one...\n";

    sceneManager.AddMesh(MeshSources::Cube(), "textures/brick.jpg");
    sceneManager.AddMesh(MeshSources::TriangularPrism(), "textures/wood.jpg");
    sceneManager.AddMesh(MeshSources::Sphere(16, 16), "textures/earth.jpg");
    sceneManager.AddLight().transform.position = {-2.0f, 1.0f, -2.0f};

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
        sceneManager.drawScene(shader, skyboxShader, materials,
                       useFPSCamera ? fpsCamera : editorCamera,
                       SCR_WIDTH,
                       SCR_HEIGHT);

        // Draw light spheres
        sceneManager.DrawLightGizmos(lightShader,
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

        // Lights and payloads are edited together: each entity appears once.
ImGui::Begin("Scene Objects");
const std::filesystem::path textureDirectory = ResolveAssetPath("textures", "textures");
std::vector<std::string> textureFiles = GetAvailableTextures(textureDirectory.string());
static int textureIndex = 0;
static int sceneObjectSortMode = 0;
const char* sceneObjectSortModes[] = {"Newest to Oldest", "Oldest to Newest"};
if (!textureFiles.empty()) textureIndex = std::min(textureIndex, int(textureFiles.size()-1));
const std::string creationTexture = textureFiles.empty() ? std::string{} : textureFiles[textureIndex];
static CreationOption selectedCreationOption = CreationOption::Cube;
static const CreationOptionOption primitive2DTypes[] = {
    {CreationOption::Triangle, "Triangle"},
    {CreationOption::Plane, "Plane"}
};
static const CreationOptionOption primitive3DTypes[] = {
    {CreationOption::Cube, "Cube"},
    {CreationOption::TriangularPrism, "Triangular Prism"},
    {CreationOption::Sphere, "Sphere"},
    {CreationOption::Slab, "Slab"}
};
static const CreationOptionOption environmentTypes[] = {
    {CreationOption::Terrain, "Terrain"},
    {CreationOption::Rock, "Rock"},
    {CreationOption::Skybox, "Skybox"}
};
static const CreationOptionOption lightingTypes[] = {
    {CreationOption::Light, "Light"}
};
const char* selectedSceneObjectName = "Cube";
for (const CreationOptionOption& option : primitive2DTypes)
    if (option.type == selectedCreationOption) selectedSceneObjectName = option.name;
for (const CreationOptionOption& option : primitive3DTypes)
    if (option.type == selectedCreationOption) selectedSceneObjectName = option.name;
for (const CreationOptionOption& option : environmentTypes)
    if (option.type == selectedCreationOption) selectedSceneObjectName = option.name;
for (const CreationOptionOption& option : lightingTypes)
    if (option.type == selectedCreationOption) selectedSceneObjectName = option.name;

ImGui::TextUnformatted("Add Object:");
const float addButtonWidth = ImGui::CalcTextSize("Add").x + ImGui::GetStyle().FramePadding.x * 2.0f;
ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - addButtonWidth - ImGui::GetStyle().ItemSpacing.x));
if (ImGui::BeginCombo("##CreationOption", selectedSceneObjectName)) {
    CreationOptionGroup("Primitives 2D", primitive2DTypes, 2, selectedCreationOption);
    ImGui::Separator();
    CreationOptionGroup("Primitives 3D", primitive3DTypes, 4, selectedCreationOption);
    ImGui::Separator();
    CreationOptionGroup("Environment", environmentTypes, 3, selectedCreationOption);
    ImGui::Separator();
    CreationOptionGroup("Lighting", lightingTypes, 1, selectedCreationOption);
    ImGui::EndCombo();
}
ImGui::SameLine();
const bool skyboxAlreadyExists = selectedCreationOption == CreationOption::Skybox && sceneManager.GetSkybox();
ImGui::BeginDisabled(skyboxAlreadyExists);
if (ImGui::Button("Add")) {
    switch (selectedCreationOption) {
        case CreationOption::Triangle: sceneManager.AddMesh(MeshSources::Triangle(), creationTexture); break;
        case CreationOption::Plane: sceneManager.AddMesh(MeshSources::Plane(), creationTexture); break;
        case CreationOption::Cube: sceneManager.AddMesh(MeshSources::Cube(), creationTexture); break;
        case CreationOption::TriangularPrism: sceneManager.AddMesh(MeshSources::TriangularPrism(), creationTexture); break;
        case CreationOption::Sphere: sceneManager.AddMesh(MeshSources::Sphere(), creationTexture); break;
        case CreationOption::Slab: sceneManager.AddMesh(MeshSources::Slab(), creationTexture); break;
        case CreationOption::Rock: sceneManager.AddMesh(MeshSources::Rock(), creationTexture); break;
        case CreationOption::Terrain: {
            auto& object = sceneManager.AddTerrain();
            std::get<Terrain>(object.payload).SetTexturePath(creationTexture);
            break;
        }
        case CreationOption::Skybox: sceneManager.AddSkybox(); break;
        case CreationOption::Light: sceneManager.AddLight(); break;
        
    }
}
ImGui::EndDisabled();
if (skyboxAlreadyExists && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    ImGui::SetTooltip("The scene can only contain one skybox.");

ImGui::Spacing();
ImGui::Separator();
ImGui::TextUnformatted("Object List:");
ImGui::TextUnformatted("Sort");
ImGui::SetNextItemWidth(-1.0f);
ImGui::Combo("##SceneObjectSort", &sceneObjectSortMode, sceneObjectSortModes, 2);

auto& objects=sceneManager.GetSceneObjects();
std::vector<std::size_t> displayOrder;
displayOrder.reserve(objects.size());
for(std::size_t index=0;index<objects.size();++index)displayOrder.push_back(index);
if(sceneObjectSortMode==0)std::reverse(displayOrder.begin(),displayOrder.end());
std::optional<std::size_t> objectToDelete;
for(std::size_t sourceIndex:displayOrder){auto& o=*objects[sourceIndex];ImGui::PushID(static_cast<int>(sourceIndex));
 const std::string objectHeaderLabel=o.name+"###ObjectHeader";
 const bool isSkybox=std::holds_alternative<Skybox>(o.payload);
 const bool renderable=!std::holds_alternative<std::monostate>(o.payload);
 const bool objectExpanded=ImGui::CollapsingHeader(objectHeaderLabel.c_str());
 if(objectExpanded){char name[128];std::snprintf(name,sizeof(name),"%s",o.name.c_str());ImGui::TextUnformatted("Name");ImGui::SetNextItemWidth(-1.0f);if(ImGui::InputText("##Name",name,sizeof(name)))o.name=name;
  if(renderable){
   if(ImGui::BeginTable("RenderSettings",2,ImGuiTableFlags_SizingStretchSame)){
    ImGui::TableNextColumn();
    int mode=int(o.renderMode);const char* modes[]={"Texture","Material"};
    ImGui::TextUnformatted("Render Mode");ImGui::SetNextItemWidth(-1.0f);
    if(ImGui::Combo("##RenderMode",&mode,modes,2))o.renderMode=RenderMode(mode);

    ImGui::TableNextColumn();
    if(o.renderMode==RenderMode::Material){
     const auto& ids=materials.GetIds();ImGui::TextUnformatted("Material");
     if(ids.empty())ImGui::TextDisabled("No materials available");
     else{std::string preview=o.materialId.empty()?"(missing)":o.materialId;ImGui::SetNextItemWidth(-1.0f);if(ImGui::BeginCombo("##Material",preview.c_str())){for(const auto&id:ids){bool selected=id==o.materialId;if(ImGui::Selectable(id.c_str(),selected))o.materialId=id;if(selected)ImGui::SetItemDefaultFocus();}ImGui::EndCombo();}}
    }else{
     std::visit([&](auto& payload){using T=std::decay_t<decltype(payload)>;if constexpr(std::is_same_v<T,MeshObject>||std::is_same_v<T,Terrain>)TextureSelector(textureFiles,payload.GetTexturePath(),[&](const std::string& texture){payload.SetTexturePath(texture);});else if constexpr(std::is_same_v<T,Skybox>)SkyboxTextureSelector(skyboxNames,payload);},o.payload);
    }
    ImGui::EndTable();
   }
   if(o.renderMode==RenderMode::Material){auto warning=materials.GetWarning(o.materialId);if(!warning.empty())ImGui::TextWrapped("Material warning: %s",warning.c_str());}
  }
  std::visit([&](auto& payload){using T=std::decay_t<decltype(payload)>;
   if constexpr(std::is_same_v<T,MeshObject>){ImGui::Text("Primitive: %s",payload.GetSourceName().c_str());}
   else if constexpr(std::is_same_v<T,Terrain>){auto& c=payload.GetConfig();ImGui::Text("Terrain: %s / %s",c.geometryType==TerrainGeometryType::Flat?"Flat":"Heightmap",c.plane==TerrainPlane::XY?"XY":c.plane==TerrainPlane::YZ?"YZ":"XZ");ImGui::Text("%d x %d segments, %.1f x %.1f",c.widthSegments,c.depthSegments,c.width,c.depth);}
   else if constexpr(std::is_same_v<T,Skybox>){
    if(o.renderMode==RenderMode::Texture){
    if(ImGui::Button("Refresh Skyboxes"))skyboxNames=SkyboxAssets::Discover();
    if(!payload.GetLastError().empty())ImGui::TextWrapped("Error: %s",payload.GetLastError().c_str());}
   }
   else ImGui::TextUnformatted("No renderable payload");},o.payload);
  if(!isSkybox)DragVector3("Position",&o.transform.position.x,.05f);DragVector3("Rotation",&o.transform.rotation.x,.5f);if(!isSkybox)DragVector3("Scale",&o.transform.scale.x,.05f,.01f,100.f);
  ImGui::TextUnformatted("Enabled");ImGui::Checkbox("##Enabled",&o.enabled);
  ImGui::TextUnformatted("Visible");ImGui::Checkbox("##Visible",&o.visible);
  if(!isSkybox&&o.light){int type=int(o.light->type);const char* types[]={"Point","Directional","Spot"};
   ImGui::TextUnformatted("Light Type");ImGui::SetNextItemWidth(-1.0f);ImGui::Combo("##LightType",&type,types,3);o.light->type=LightType(type);
   ImGui::TextUnformatted("Light Color");ImGui::SetNextItemWidth(-1.0f);ImGui::ColorEdit3("##LightColor",&o.light->color.x);
   ImGui::TextUnformatted("Intensity");ImGui::SetNextItemWidth(-1.0f);ImGui::DragFloat("##Intensity",&o.light->intensity,.05f,0);
   ImGui::TextUnformatted("Range");ImGui::SetNextItemWidth(-1.0f);ImGui::DragFloat("##Range",&o.light->range,.1f,0);
   ImGui::TextUnformatted("Inner Cone");ImGui::SetNextItemWidth(-1.0f);ImGui::DragFloat("##InnerCone",&o.light->innerConeAngle,.5f,0,180);
   ImGui::TextUnformatted("Outer Cone");ImGui::SetNextItemWidth(-1.0f);ImGui::DragFloat("##OuterCone",&o.light->outerConeAngle,.5f,0,180);
   if(ImGui::Button("Remove Light Component"))o.light.reset();}
  else if(!isSkybox&&!o.light&&ImGui::Button("Add Light Component"))o.light.emplace();
  if(ImGui::Button("Delete Object"))objectToDelete=sourceIndex;
 }ImGui::PopID();}
if(objectToDelete)sceneManager.RemoveSceneObject(*objectToDelete);
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
    materials.Clear();
    skyboxShader = Shader{};
    lightShader = Shader{};
    shader = Shader{};
    boundingBoxShader = Shader{};
    glfwTerminate();
    return 0;
}
