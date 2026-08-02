#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(const MeshData& data) { Upload(data); }
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void Upload(const MeshData& data);
    void Draw() const;

private:
    void Destroy();
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
};

bool IntersectsSphereMesh(const glm::vec3& sphereCenter, float sphereRadius,
                          const MeshData& geometry, const glm::mat4& model,
                          glm::vec3* contactNormal = nullptr);
