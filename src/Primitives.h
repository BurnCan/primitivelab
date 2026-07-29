#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp> // For modelMatrix
#include <glm/gtc/matrix_transform.hpp>
#include "SceneObject.h"


// ---- How to Add a New Primitive ----
// 1. Add a value to the appropriate TwoD/ThreeD PrimitiveType enum and to
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

namespace TwoD {
enum class PrimitiveType { Triangle, Plane };

class Primitive final : public SceneObject, public detail::PrimitiveMesh {
public:
    Primitive(PrimitiveType type, const std::string& texturePath)
        : PrimitiveMesh(type == PrimitiveType::Triangle
                            ? detail::PrimitiveMeshType::Triangle
                            : detail::PrimitiveMeshType::Plane,
                        texturePath) {}
    std::string GetName() const final { return GetTypeName(); }
    PrimitiveType GetPrimitiveType() const {
        return GetType() == detail::PrimitiveMeshType::Triangle
            ? PrimitiveType::Triangle : PrimitiveType::Plane;
    }
};
} // namespace TwoD

namespace ThreeD {
enum class PrimitiveType { Cube, TriangularPrism, Sphere, Slab };

class Primitive final : public SceneObject, public detail::PrimitiveMesh {
public:
    Primitive(PrimitiveType type, const std::string& texturePath,
              unsigned int xSegments = 32, unsigned int ySegments = 32)
        : PrimitiveMesh(ToMeshType(type), texturePath, xSegments, ySegments) {}
    std::string GetName() const final { return GetTypeName(); }
    PrimitiveType GetPrimitiveType() const;

private:
    static detail::PrimitiveMeshType ToMeshType(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::Cube: return detail::PrimitiveMeshType::Cube;
            case PrimitiveType::TriangularPrism: return detail::PrimitiveMeshType::TriangularPrism;
            case PrimitiveType::Sphere: return detail::PrimitiveMeshType::Sphere;
            case PrimitiveType::Slab: return detail::PrimitiveMeshType::Slab;
        }
        return detail::PrimitiveMeshType::Cube;
    }
};

inline PrimitiveType Primitive::GetPrimitiveType() const {
    switch (GetType()) {
        case detail::PrimitiveMeshType::TriangularPrism: return PrimitiveType::TriangularPrism;
        case detail::PrimitiveMeshType::Sphere: return PrimitiveType::Sphere;
        case detail::PrimitiveMeshType::Slab: return PrimitiveType::Slab;
        default: return PrimitiveType::Cube;
    }
}
} // namespace ThreeD
