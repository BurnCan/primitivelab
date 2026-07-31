// ---------------- Terrain.h ----------------
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

enum class TerrainGeometryType { Flat, Heightmap };
enum class TerrainPlane { XY, XZ, YZ };
struct TerrainConfig {
    TerrainGeometryType geometryType = TerrainGeometryType::Flat;
    TerrainPlane plane = TerrainPlane::XZ;
    int widthSegments = 20, depthSegments = 20;
    float width = 20.0f, depth = 20.0f, heightScale = 1.0f;
    std::string heightmapPath;
};

class Terrain {
public:
    explicit Terrain(const TerrainConfig& config = {});
    Terrain(int width, int depth, float scale);
    ~Terrain();
    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    void Draw(GLuint shaderProgram);
    void DrawWireframe() const;
    bool IntersectsSphere(const glm::vec3& center, float radius,
                          glm::vec3* contactNormal = nullptr,
                          const glm::mat4& model = glm::mat4(1.0f)) const;
    void SetTexturePath(const std::string& path);
    const std::string& GetTexturePath() const { return texturePath; }

    // ✅ Add this getter so SceneManager can bind the texture
    GLuint GetTextureID() const { return textureID; }
    const TerrainConfig& GetConfig() const { return config; }

private:
    void GenerateMesh();
    TerrainConfig config;

    GLuint VAO, VBO, EBO;
    GLuint textureID;
    int indexCount;
    std::string texturePath;
    std::vector<glm::vec3> collisionVertices;
    std::vector<unsigned int> collisionIndices;
};
