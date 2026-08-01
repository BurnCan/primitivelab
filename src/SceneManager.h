#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "Camera.h"
#include "SceneObject.h"
#include "ShaderUtils.h"
#include "MaterialRegistry.h"

class SceneManager {
public:
    SceneManager();
    bool loadScene(const std::string& filepath);
    bool saveScene(const std::string& filepath);
    void drawScene(const Shader&, const Shader&, const MaterialRegistry&, Camera&, int, int);
    void DrawLightGizmos(const Shader&, Camera&, int, int);
    bool CheckCollision(const glm::vec3&, float radius = FPS_CAMERA_COLLISION_RADIUS, glm::vec3* = nullptr);
    void DrawDebugWindow();
    void DrawCollisionMeshes(const Shader&, const Camera&, int, int, bool = true, bool = false);

    const std::vector<std::unique_ptr<SceneObject>>& GetSceneObjects() const { return sceneObjects; }
    std::vector<std::unique_ptr<SceneObject>>& GetSceneObjects() { return sceneObjects; }
    bool RemoveSceneObject(std::size_t);
    SceneObject& AddPrimitive2D(Primitive2DType, const std::string&);
    SceneObject& AddPrimitive3D(Primitive3DType, const std::string&, unsigned = 32, unsigned = 32);
    SceneObject& AddTerrain(const TerrainConfig& = {});
    SceneObject& AddSkybox(const SkyboxConfig& = {});
    SceneObject* GetSkybox();
    const SceneObject* GetSkybox() const;
    SceneObject& AddLight();
    void Clear() { sceneObjects.clear(); }
private:
    std::vector<std::unique_ptr<SceneObject>> sceneObjects;
    Primitive3D lightGizmoSphere{Primitive3DType::Sphere, "sun.jpg", 16, 16};
};
