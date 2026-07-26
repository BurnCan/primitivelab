// ---------------- Terrain.h ----------------
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Terrain {
public:
    Terrain(int width = 10, int depth = 10, float scale = 1.0f);

    void Draw(GLuint shaderProgram);
    void DrawWireframe() const;
    bool IntersectsSphere(const glm::vec3& center, float radius,
                          glm::vec3* contactNormal = nullptr) const;
    void SetTexturePath(const std::string& path);
    const std::string& GetTexturePath() const { return texturePath; }

    // ✅ Add this getter so SceneManager can bind the texture
    GLuint GetTextureID() const { return textureID; }

private:
    void GenerateMesh(int width, int depth, float scale);

    GLuint VAO, VBO, EBO;
    GLuint textureID;
    int indexCount;
    std::string texturePath;
    std::vector<glm::vec3> collisionVertices;
    std::vector<unsigned int> collisionIndices;
};
