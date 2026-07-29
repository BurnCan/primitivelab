#pragma once
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Primitives.h"
#include "Camera.h"
#include "Terrain.h"
#include "ShaderUtils.h"

struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

class Camera; // 👈 Add this forward declaration

class SceneManager {
public:
    SceneManager();

    // Scene I/O
    bool loadScene(const std::string& filepath);
    bool saveScene(const std::string& filepath);

    // Drawing
    void drawScene(const Shader& shader, Camera& camera, int SCR_WIDTH, int SCR_HEIGHT);
    void drawLights(const Shader& lightShader, Camera& camera, int SCR_WIDTH, int SCR_HEIGHT);


    // --- Collision ---
    bool CheckCollision(const glm::vec3& point, float radius = FPS_CAMERA_COLLISION_RADIUS,
                        glm::vec3* contactNormal = nullptr);


    void DrawDebugWindow();
    void DrawCollisionMeshes(const Shader& shader, const Camera& camera, int width, int height,
                             bool drawPrimitiveMeshes = true, bool drawTerrainMesh = false);



    // Scene management
    const std::vector<std::unique_ptr<SceneObject>>& GetSceneObjects() const { return sceneObjects; }
    void AddSceneObject(std::unique_ptr<SceneObject> object) { sceneObjects.push_back(std::move(object)); }
    bool RemoveSceneObject(std::size_t index);

     // Terrain access
    Terrain& GetTerrain() { return terrain; }
    const Terrain& GetTerrain() const { return terrain; }

    // Lights
    const std::vector<Light>& GetLights() const { return lights; } // read-only
    std::vector<Light>& GetLights() { return lights; }             // editable

    // Clear all owned scene objects and lights.
    void Clear() {
    sceneObjects.clear();
    lights.clear();
    }
    void AddLight(const Light& light) { lights.push_back(light); }

private:
    std::vector<std::unique_ptr<SceneObject>> sceneObjects;
    std::vector<Light> lights;
    ThreeD::Primitive lightSphere { ThreeD::PrimitiveType::Sphere, "sun.jpg" }; // reusable light sphere
    std::string currentSceneFile; // ✅ track the currently opened scene
    Terrain terrain;   // member terrain
};
