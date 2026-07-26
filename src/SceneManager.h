#pragma once
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

struct AABB {
    glm::vec3 min;
    glm::vec3 max;



   bool IntersectsSphere(const glm::vec3& center, float radius) const {
    // Clamp sphere center to box boundaries
    glm::vec3 closest = glm::clamp(center, min, max);

    // Compute squared distance manually (faster than glm::distance)
    glm::vec3 diff = center - closest;
    float distanceSquared = glm::dot(diff, diff);

    return distanceSquared < (radius * radius);
}
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
    bool CheckCollision(const glm::vec3& point, float radius = 0.1f);


    void DrawDebugWindow();
    bool showBoundingBoxes = false; // Debug toggle
    void DrawBoundingBoxes(const Shader& shader, const Camera& camera, int width, int height);



    // Scene management
    const std::vector<std::unique_ptr<Primitive>>& GetPrimitives() const { return primitives; }
    void AddPrimitive(std::unique_ptr<Primitive> prim) { primitives.push_back(std::move(prim)); }

     // Terrain access
    Terrain& GetTerrain() { return terrain; }
    const Terrain& GetTerrain() const { return terrain; }

    // Lights
    const std::vector<Light>& GetLights() const { return lights; } // read-only
    std::vector<Light>& GetLights() { return lights; }             // editable

    //Clear lights and primitives
    void Clear() {
    primitives.clear();
    lights.clear();
    }
     void UpdateBoundingBoxes();
    void AddLight(const Light& light) { lights.push_back(light); }

private:
    std::vector<std::unique_ptr<Primitive>> primitives;
    std::vector<Light> lights;
    Primitive lightSphere { PrimitiveType::Sphere, "sun.jpg" }; // reusable light sphere
    std::string currentSceneFile; // ✅ track the currently opened scene
    Terrain terrain;   // member terrain
    std::vector<AABB> boundingBoxes;


    GLuint debugVAO = 0, debugVBO = 0;
    void InitDebugCube();

};
