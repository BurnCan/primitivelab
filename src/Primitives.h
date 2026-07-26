#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp> // For modelMatrix
#include <glm/gtc/matrix_transform.hpp>


// ---- How to Add a New Primitive ----
// 1. Add a new value to the PrimitiveType enum in Primitives.h.
//    Example: TriangularPrism
//
// 2. Declare a setup function for the new primitive in the Primitive class.
//    Example: void setupTriangularPrism();
//
// 3. Implement the setup function in Primitives.cpp:
//    - Define the vertices (positions, normals, texcoords) for your shape.
//    - Define the indices for glDrawElements.
//    - Call setupVAO(vertices, indices, stride); (stride = 3 for position-only, 8 for pos+norm+tex)
//
// 4. Update constructShapeMesh() to handle the new PrimitiveType value
//    and call the corresponding setup function.
//
// 5. Optionally, add a constructor overload if special parameters (like segments) are needed.

// Utility: list all available textures in a directory
std::vector<std::string> GetAvailableTextures(const std::string& dir);

enum class PrimitiveType {
    Cube,
    TriangularPrism,
    Sphere,
    Triangle,
    Plane,
    Slab
};

class Primitive {
public:
    // Public getters
    GLuint GetTextureID() const { return textureID; }
    const std::string& GetTexturePath() const { return texturePath; }
    PrimitiveType GetType() const { return type; }

    // New: get readable type name for UI
    std::string GetTypeName() const {
        switch (type) {
            case PrimitiveType::Cube:            return "Cube";
            case PrimitiveType::TriangularPrism: return "Triangular Prism";
            case PrimitiveType::Sphere:          return "Sphere";
            case PrimitiveType::Triangle:        return "Triangle";
            case PrimitiveType::Plane:        return "Plane";
            case PrimitiveType::Slab:        return "Slab";
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
    Primitive(PrimitiveType type, const std::string& texturePath);
    Primitive(PrimitiveType type, const std::string& texturePath,
              unsigned int xSeg, unsigned int ySeg);
    ~Primitive();
    // Draw
    void draw() const;
    void drawWireframe() const;

    // Tests a world-space sphere against the actual triangles in this mesh.
    bool IntersectsSphere(const glm::vec3& center, float radius) const;

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
    PrimitiveType type;
    std::string texturePath;
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLuint textureID = 0;
    GLsizei indexCount = 0;

    std::vector<float> localVertices; // stores full vertex data (x,y,z,nx,ny,nz,u,v)
    std::vector<unsigned int> localIndices;

    unsigned int X_SEGMENTS = 32;
    unsigned int Y_SEGMENTS = 32;
};
