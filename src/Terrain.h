// ---------------- Terrain.h ----------------
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include "MeshObject.h"

using TerrainConfig = TerrainMeshSource;

namespace TerrainGeometry { MeshData CreateMeshData(const TerrainConfig& config); }

class Terrain {
public:
    explicit Terrain(const TerrainConfig& config = {});
    Terrain(int width, int depth, float scale);
    ~Terrain();
    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    void Draw();
    void DrawWireframe() const;
    bool IntersectsSphere(const glm::vec3& center, float radius,
                          glm::vec3* contactNormal = nullptr,
                          const glm::mat4& model = glm::mat4(1.0f)) const;
    void SetTexturePath(const std::string& path);
    const std::string& GetTexturePath() const { return object.GetTexturePath(); }

    // ✅ Add this getter so SceneManager can bind the texture
    GLuint GetTextureID() const { return object.GetTextureID(); }
    const TerrainConfig& GetConfig() const { return config; }

private:
    TerrainConfig config;

    MeshObject object;
};
