#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp> // For modelMatrix
#include <glm/gtc/matrix_transform.hpp>
#include "SceneObject.h"


// ---- How to Add a New Primitive ----
// 1. Add a value to the appropriate Primitive2DType/Primitive3DType enum and to
//    the internal PrimitiveMeshType used by mesh generation.
//    Example: TriangularPrism
//
// 2. Declare a setup function on PrimitiveMesh.
//    Example: void setupTriangularPrism();
//
// 3. Implement the setup function in Primitives.cpp:
//    - Define the vertices (positions, normals, texcoords) for your shape.
//    - Define the indices for glDrawElements.
//    - Call setupVAO(vertices, indices, stride); (stride = 3 for position-only, 8 for pos+norm+tex)
//
// 4. Update constructShapeMesh() to handle the new PrimitiveMeshType value
//    and call the corresponding setup function.
//
// 5. Optionally, add a constructor overload if special parameters (like segments) are needed.

// Utility: list all available textures in a directory
std::vector<std::string> GetAvailableTextures(const std::string& dir);

namespace detail {
enum class PrimitiveMeshType {
    Cube,
    TriangularPrism,
    Sphere,
    Triangle,
    Plane,
    Slab
};

class PrimitiveMesh {
public:
    // Public getters
    GLuint GetTextureID() const { return textureID; }
    const std::string& GetTexturePath() const { return texturePath; }
    PrimitiveMeshType GetType() const { return type; }

    // New: get readable type name for UI
    std::string GetTypeName() const {
        switch (type) {
            case PrimitiveMeshType::Cube:            return "Cube";
            case PrimitiveMeshType::TriangularPrism: return "Triangular Prism";
            case PrimitiveMeshType::Sphere:          return "Sphere";
            case PrimitiveMeshType::Triangle:        return "Triangle";
            case PrimitiveMeshType::Plane:           return "Plane";
            case PrimitiveMeshType::Slab:            return "Slab";
            default:                             return "Unknown";
        }
    }

    // Public model matrix for rendering
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // New: transform components (editable in ImGui)
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation {0.0f, 0.0f, 0.0f}; // Euler angles (degrees)
    glm::vec3 scale    {1.0f, 1.0f, 1.0f};

    // Constructors
    PrimitiveMesh(PrimitiveMeshType type, const std::string& texturePath);
    PrimitiveMesh(PrimitiveMeshType type, const std::string& texturePath,
              unsigned int xSeg, unsigned int ySeg);
    ~PrimitiveMesh();
    // Draw
    void draw() const;
    void drawWireframe() const;

    // Tests a world-space sphere against the actual triangles in this mesh.
    // When supplied, contactNormal receives the direction from the closest
    // surface point toward the sphere center.
    bool IntersectsSphere(const glm::vec3& center, float radius,
                          glm::vec3* contactNormal = nullptr) const;

    // New: update model matrix from transform components
    void UpdateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));

        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));

        modelMatrix = glm::scale(modelMatrix, scale);

    }

    // Texture management
    void SetTexturePath(const std::string& newPath);
    void ReloadTexture();  // reload using current texturePath

private:
    // Geometry dispatcher + setup
    void constructShapeMesh();
    void setupCube();
    void setupSphere();
    void setupTriangle();
    void setupTriangularPrism();
    void setupPlane();
    void setupSlab();

    // Shared VAO/VBO/EBO setup
    void setupVAO(const std::vector<float>& vertices,
                  const std::vector<unsigned int>& indices,
                  int stride = 8);

    // Texture loading
    void loadTexture(const std::string& path);

    // Members
    PrimitiveMeshType type;
    std::string texturePath;
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLuint textureID = 0;
    GLsizei indexCount = 0;

    std::vector<float> localVertices; // stores full vertex data (x,y,z,nx,ny,nz,u,v)
    std::vector<unsigned int> localIndices;

    unsigned int X_SEGMENTS = 32;
    unsigned int Y_SEGMENTS = 32;
};
} // namespace detail

enum class Primitive2DType { Triangle, Plane };

class Primitive2D final : public SceneObject, public detail::PrimitiveMesh {
public:
    Primitive2D(Primitive2DType type, const std::string& texturePath)
        : PrimitiveMesh(type == Primitive2DType::Triangle
                            ? detail::PrimitiveMeshType::Triangle
                            : detail::PrimitiveMeshType::Plane,
                        texturePath) {}
    std::string GetName() const final { return GetTypeName(); }
    Primitive2DType GetPrimitiveType() const {
        return GetType() == detail::PrimitiveMeshType::Triangle
            ? Primitive2DType::Triangle : Primitive2DType::Plane;
    }
};

enum class Primitive3DType { Cube, TriangularPrism, Sphere, Slab };

class Primitive3D final : public SceneObject, public detail::PrimitiveMesh {
public:
    Primitive3D(Primitive3DType type, const std::string& texturePath,
              unsigned int xSegments = 32, unsigned int ySegments = 32)
        : PrimitiveMesh(ToMeshType(type), texturePath, xSegments, ySegments) {}
    std::string GetName() const final { return GetTypeName(); }
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
