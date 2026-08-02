#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp> // For modelMatrix
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"


// PrimitiveGeometry produces CPU-side indexed geometry without invoking OpenGL.
// PrimitiveMesh retains the public scene API while delegating rendering to Mesh.

// Utility: list all available textures in a directory
std::vector<std::string> GetAvailableTextures(const std::string& dir);

namespace detail {
enum class PrimitiveMeshType { Cube, TriangularPrism, Sphere, Triangle, Plane, Slab };

namespace PrimitiveGeometry {
MeshData CreateTriangle();
MeshData CreateCube();
MeshData CreateSphere(unsigned int xSegments = 32, unsigned int ySegments = 32);
MeshData CreateTriangularPrism();
MeshData CreatePlane();
MeshData CreateSlab();
MeshData Create(PrimitiveMeshType type, unsigned int xSegments = 32, unsigned int ySegments = 32);
}

class PrimitiveMesh {
public:
    PrimitiveMesh(const PrimitiveMesh&) = delete;
    PrimitiveMesh& operator=(const PrimitiveMesh&) = delete;
    PrimitiveMesh(PrimitiveMeshType type, const std::string& texturePath);
    PrimitiveMesh(PrimitiveMeshType type, const std::string& texturePath, unsigned int xSeg, unsigned int ySeg);
    ~PrimitiveMesh();
    GLuint GetTextureID() const { return textureID; }
    const std::string& GetTexturePath() const { return texturePath; }
    PrimitiveMeshType GetType() const { return type; }
    std::string GetTypeName() const;
    glm::mat4 modelMatrix{1.0f};
    void draw() const;
    void drawWireframe() const;
    bool IntersectsSphere(const glm::vec3& center, float radius, glm::vec3* contactNormal = nullptr) const;
    void SetTexturePath(const std::string& newPath);
    void ReloadTexture();
private:
    void loadTexture(const std::string& path);
    PrimitiveMeshType type;
    std::string texturePath;
    GLuint textureID = 0;
    MeshData geometry;
    Mesh mesh;
    unsigned int X_SEGMENTS = 32, Y_SEGMENTS = 32;
};
} // namespace detail

enum class Primitive2DType { Triangle, Plane };

class Primitive2D final : public detail::PrimitiveMesh {
public:
    Primitive2D(Primitive2DType type, const std::string& texturePath)
        : PrimitiveMesh(type == Primitive2DType::Triangle
                            ? detail::PrimitiveMeshType::Triangle
                            : detail::PrimitiveMeshType::Plane,
                        texturePath) {}
    Primitive2DType GetPrimitiveType() const {
        return GetType() == detail::PrimitiveMeshType::Triangle
            ? Primitive2DType::Triangle : Primitive2DType::Plane;
    }
};

enum class Primitive3DType { Cube, TriangularPrism, Sphere, Slab };

class Primitive3D final : public detail::PrimitiveMesh {
public:
    Primitive3D(Primitive3DType type, const std::string& texturePath,
              unsigned int xSegments = 32, unsigned int ySegments = 32)
        : PrimitiveMesh(ToMeshType(type), texturePath, xSegments, ySegments) {}
    Primitive3DType GetPrimitiveType() const;

private:
    static detail::PrimitiveMeshType ToMeshType(Primitive3DType type) {
        switch (type) {
            case Primitive3DType::Cube: return detail::PrimitiveMeshType::Cube;
            case Primitive3DType::TriangularPrism: return detail::PrimitiveMeshType::TriangularPrism;
            case Primitive3DType::Sphere: return detail::PrimitiveMeshType::Sphere;
            case Primitive3DType::Slab: return detail::PrimitiveMeshType::Slab;
        }
        return detail::PrimitiveMeshType::Cube;
    }
};

inline Primitive3DType Primitive3D::GetPrimitiveType() const {
    switch (GetType()) {
        case detail::PrimitiveMeshType::TriangularPrism: return Primitive3DType::TriangularPrism;
        case detail::PrimitiveMeshType::Sphere: return Primitive3DType::Sphere;
        case detail::PrimitiveMeshType::Slab: return Primitive3DType::Slab;
        default: return Primitive3DType::Cube;
    }
}
