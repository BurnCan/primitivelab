#pragma once

#include <string>
#include <glm/glm.hpp>
#include "MeshFactory.h"

class MeshObject {
public:
    MeshObject(MeshSource source, const std::string& texturePath = {});
    ~MeshObject();
    MeshObject(const MeshObject&) = delete;
    MeshObject& operator=(const MeshObject&) = delete;
    MeshObject(MeshObject&& other) noexcept;
    MeshObject& operator=(MeshObject&& other) noexcept;

    void Draw() const;
    void DrawWireframe() const;
    bool IntersectsSphere(const glm::vec3& center, float radius,
                          glm::vec3* contactNormal = nullptr) const;
    const MeshSource& GetMeshSource() const { return source; }
    const MeshData& GetMeshData() const { return geometry; }
    GLuint GetTextureID() const { return textureID; }
    const std::string& GetTexturePath() const { return texturePath; }
    std::string GetSourceName() const;
    void SetTexturePath(const std::string& path);
    void ReloadTexture();

    glm::mat4 modelMatrix{1.0f};

private:
    void LoadTexture();
    MeshSource source;
    MeshData geometry;
    Mesh mesh;
    std::string texturePath;
    GLuint textureID = 0;
};
