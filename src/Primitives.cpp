#include "Primitives.h"
#include <stb_image.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "AssetPaths.h"
namespace fs = std::filesystem;

// Utility: list all available textures in a directory
std::vector<std::string> GetAvailableTextures(const std::string& dir) {
    std::vector<std::string> textures;

    try {
        for (auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    textures.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning texture directory " << dir << ": " << e.what() << std::endl;
    }

    std::sort(textures.begin(), textures.end());
    return textures;
}

void Primitive::SetTexturePath(const std::string& newPath) {
    // Normalize to filename only
    fs::path cleanPath = fs::path(newPath).filename();
    texturePath = cleanPath.string();
    ReloadTexture();
}

void Primitive::ReloadTexture() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    if (!texturePath.empty()) {
        loadTexture(texturePath);
    }
}

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

// ---------------- Constructors ----------------
Primitive::Primitive(PrimitiveType type, const std::string& texturePath)
    : type(type)
{
    // Normalize: always use only the filename
    fs::path cleanPath = fs::path(texturePath).filename();
    this->texturePath = cleanPath.string();

    constructShapeMesh();
    loadTexture(this->texturePath);
}

Primitive::Primitive(PrimitiveType type, const std::string& texturePath, unsigned int xSeg, unsigned int ySeg)
    : type(type), X_SEGMENTS(xSeg), Y_SEGMENTS(ySeg)
{
    // Normalize: always use only the filename
    fs::path cleanPath = fs::path(texturePath).filename();
    this->texturePath = cleanPath.string();

    constructShapeMesh();
    loadTexture(this->texturePath);
}


// ---------------- Destructor ----------------
Primitive::~Primitive() {
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (textureID) glDeleteTextures(1, &textureID);
}


// ---------------- Mesh Construction Dispatcher ----------------
// Selects the appropriate setup function to generate vertex/index buffers
// and configure the VAO/VBO/EBO for this primitive type.
//void Primitive::constructShapeMesh() {
//    switch (type) {
//        case PrimitiveType::Cube:
//            setupCube();
//            std::cout << "[DEBUG] Constructed Cube: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//        case PrimitiveType::Sphere:
//            setupSphere();
//            std::cout << "[DEBUG] Constructed Sphere: VAO=" << VAO
//                      << ", indexCount=" << indexCount
//                      << ", Segments=" << X_SEGMENTS << "x" << Y_SEGMENTS << std::endl;
//            break;
//        case PrimitiveType::TriangularPrism:
//            setupTriangularPrism();
//            std::cout << "[DEBUG] Constructed TriangularPrism: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//        case PrimitiveType::Triangle:
 //           setupTriangle();
//            std::cout << "[DEBUG] Constructed Triangle: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//            case PrimitiveType::Plane:
//            setupPlane();
//            std::cout << "[DEBUG] Constructed Plane: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//            case PrimitiveType::Slab:
//            setupSlab();
//            std::cout << "[DEBUG] Constructed Slab: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//        default:
 //           std::cerr << "[DEBUG] Unknown primitive type!" << std::endl;
//            break;
//    }

    // Optional: print texture ID if already loaded (0 if not yet)
 //   std::cout << "[DEBUG] TextureID=" << textureID << std::endl;
//}

// -----------------------------------------------------------------------------
// Update all setupXXX() functions to fill localVertices (not a local variable)
// -----------------------------------------------------------------------------

// ---------------- Shared VAO setup ----------------
// Configures the VAO/VBO/EBO for the primitive.
// 'stride' specifies the number of floats per vertex:
//   3 = positions only (used by Triangle)
//   8 = positions + normals + texcoords (used by Cube and Sphere)
void Primitive::setupVAO(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, int stride) {
    indexCount = static_cast<GLsizei>(indices.size());
    localIndices = indices;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    if (stride == 3) {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    } else if (stride == 8) {
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texcoords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(0);
}


// ---------------- Triangle setup ----------------
void Primitive::setupTriangle() {
    //std::vector<float> vertices = {
    localVertices = {
        // positions       // normals       // texcoords
         0.0f,  0.5f, 0.0f, 0,0,1, 0.5f,1.0f, // top
        -0.5f, -0.5f, 0.0f, 0,0,1, 0.0f,0.0f, // bottom left
         0.5f, -0.5f, 0.0f, 0,0,1, 1.0f,0.0f  // bottom right
    };

    std::vector<unsigned int> indices = { 0, 1, 2 };

    setupVAO(localVertices, indices); // use the shared setup
}

// ---------------- Plane setup ----------------
void Primitive::setupPlane() {
    float h = 0.5f; // half size, makes a 1x1 unit plane

    //std::vector<float> vertices = {
    localVertices = {
        // positions            // normals        // texcoords
        -h, 0.0f, -h,           0,1,0,           0.0f, 0.0f,
         h, 0.0f, -h,           0,1,0,           1.0f, 0.0f,
         h, 0.0f,  h,           0,1,0,           1.0f, 1.0f,
        -h, 0.0f,  h,           0,1,0,           0.0f, 1.0f
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    setupVAO(localVertices, indices, 8);
}

// ---------------- Slab setup ----------------
void Primitive::setupSlab() {
    float width  = 1.0f;
    float depth  = 1.0f;
    float height = 0.1f;

    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    float hh = height * 0.5f;

    //std::vector<float> vertices = {
    localVertices = {
        // positions                // normals          // texcoords
        // Top face (uses X/Z)
        -hw,  hh, -hd,              0, 1, 0,           0.0f,    0.0f,
         hw,  hh, -hd,              0, 1, 0,           width,   0.0f,
         hw,  hh,  hd,              0, 1, 0,           width,   depth,
        -hw,  hh,  hd,              0, 1, 0,           0.0f,    depth,

        // Bottom face (uses X/Z)
        -hw, -hh, -hd,              0,-1, 0,           0.0f,    0.0f,
         hw, -hh, -hd,              0,-1, 0,           width,   0.0f,
         hw, -hh,  hd,              0,-1, 0,           width,   depth,
        -hw, -hh,  hd,              0,-1, 0,           0.0f,    depth,

        // Front face (uses X/Y)
        -hw, -hh,  hd,              0, 0, 1,           0.0f,    0.0f,
         hw, -hh,  hd,              0, 0, 1,           width,   0.0f,
         hw,  hh,  hd,              0, 0, 1,           width,   height,
        -hw,  hh,  hd,              0, 0, 1,           0.0f,    height,

        // Back face (uses X/Y)
        -hw, -hh, -hd,              0, 0,-1,           0.0f,    0.0f,
         hw, -hh, -hd,              0, 0,-1,           width,   0.0f,
         hw,  hh, -hd,              0, 0,-1,           width,   height,
        -hw,  hh, -hd,              0, 0,-1,           0.0f,    height,

        // Left face (uses Z/Y)
        -hw, -hh, -hd,             -1, 0, 0,           0.0f,    0.0f,
        -hw, -hh,  hd,             -1, 0, 0,           depth,   0.0f,
        -hw,  hh,  hd,             -1, 0, 0,           depth,   height,
        -hw,  hh, -hd,             -1, 0, 0,           0.0f,    height,

        // Right face (uses Z/Y)
         hw, -hh, -hd,              1, 0, 0,           0.0f,    0.0f,
         hw, -hh,  hd,              1, 0, 0,           depth,   0.0f,
         hw,  hh,  hd,              1, 0, 0,           depth,   height,
         hw,  hh, -hd,              1, 0, 0,           0.0f,    height,
    };

    // 6 faces × 2 triangles = 12 triangles = 36 indices
    std::vector<unsigned int> indices;
    for (int i = 0; i < 6; ++i) {
        int start = i * 4;
        indices.push_back(start + 0);
        indices.push_back(start + 1);
        indices.push_back(start + 2);
        indices.push_back(start + 2);
        indices.push_back(start + 3);
        indices.push_back(start + 0);
    }

    setupVAO(localVertices, indices, 8);
}



// ---------------- Cube setup ----------------
void Primitive::setupCube() {
    //std::vector<float> vertices = {
    localVertices = {
        // positions         // normals        // texcoords
        -0.5f,-0.5f,0.5f, 0,0,1, 0,0,
         0.5f,-0.5f,0.5f, 0,0,1, 1,0,
         0.5f,0.5f,0.5f, 0,0,1, 1,1,
        -0.5f,0.5f,0.5f, 0,0,1, 0,1,
        -0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
         0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
         0.5f,0.5f,-0.5f, 0,0,-1, 0,1,
        -0.5f,0.5f,-0.5f, 0,0,-1, 1,1,
        -0.5f,-0.5f,-0.5f, -1,0,0, 1,0,
        -0.5f,0.5f,-0.5f, -1,0,0, 1,1,
        -0.5f,0.5f,0.5f, -1,0,0, 0,1,
        -0.5f,-0.5f,0.5f, -1,0,0, 0,0,
         0.5f,-0.5f,-0.5f, 1,0,0, 0,0,
         0.5f,0.5f,-0.5f, 1,0,0, 0,1,
         0.5f,0.5f,0.5f, 1,0,0, 1,1,
         0.5f,-0.5f,0.5f, 1,0,0, 1,0,
        -0.5f,0.5f,-0.5f, 0,1,0, 0,1,
         0.5f,0.5f,-0.5f, 0,1,0, 1,1,
         0.5f,0.5f,0.5f, 0,1,0, 1,0,
        -0.5f,0.5f,0.5f, 0,1,0, 0,0,
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,
         0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,
         0.5f,-0.5f,0.5f, 0,-1,0, 1,1,
        -0.5f,-0.5f,0.5f, 0,-1,0, 0,1
    };

    std::vector<unsigned int> indices = {
        0,1,2,2,3,0,      // Front
        4,5,6,6,7,4,      // Back
        8,9,10,10,11,8,   // Left
        12,13,14,14,15,12,// Right
        16,17,18,18,19,16,// Top
        20,21,22,22,23,20 // Bottom
    };

    setupVAO(localVertices, indices, 8);
}

// ---------------- Sphere setup ----------------
void Primitive::setupSphere() {
    localVertices.clear();
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            // position (x,y,z), normal (x,y,z), texcoord (u,v)
            localVertices.push_back(xPos);
            localVertices.push_back(yPos);
            localVertices.push_back(zPos);
            localVertices.push_back(xPos); // normal x
            localVertices.push_back(yPos); // normal y
            localVertices.push_back(zPos); // normal z
            localVertices.push_back(xSegment);
            localVertices.push_back(ySegment);
        }
    }

    // Build triangles: each quad -> two triangles (a,b,c) + (a,c,d)
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            unsigned int a = y       * (X_SEGMENTS + 1) + x;
            unsigned int b = (y + 1) * (X_SEGMENTS + 1) + x;
            unsigned int c = (y + 1) * (X_SEGMENTS + 1) + (x + 1);
            unsigned int d = y       * (X_SEGMENTS + 1) + (x + 1);

            // first triangle a,b,c
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            // second triangle a,c,d
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    setupVAO(localVertices, indices, 8);
}




void Primitive::setupTriangularPrism() {
    float h = 1.0f;        // prism height along Y
    float halfBase = 0.5f; // triangle base along X
    float halfDepth = 0.5f; // triangle depth along Z

    // Triangle side lengths (isosceles)
    float side1 = sqrt(pow(halfBase,2) + pow(halfDepth*2,2)); // slant edge
    float side2 = side1;
    float side3 = halfBase * 2.0f; // base edge
    float perimeter = side1 + side2 + side3;

    // cumulative U offsets
    float u0 = 0.0f;
    float u1 = side1 / perimeter;
    float u2 = (side1 + side2) / perimeter;
    float u3 = 1.0f; // end of strip

    //std::vector<float> vertices = {
    localVertices = {
        // Top triangle
         0.0f,  h/2,  halfDepth, 0,0,0, 0.5f, 1.0f,
        -halfBase, h/2, -halfDepth, 0,0,0, 0.0f, 0.0f,
         halfBase, h/2, -halfDepth, 0,0,0, 1.0f, 0.0f,

        // Bottom triangle
         0.0f, -h/2,  halfDepth, 0,0,0, 0.5f, 1.0f,
        -halfBase, -h/2, -halfDepth, 0,0,0, 0.0f, 0.0f,
         halfBase, -h/2, -halfDepth, 0,0,0, 1.0f, 0.0f,

        // Side 1 (front edge)
         0.0f,  h/2,  halfDepth, 0,0,0, u0, 1.0f,
         halfBase, h/2, -halfDepth, 0,0,0, u1, 1.0f,
         halfBase, -h/2, -halfDepth, 0,0,0, u1, 0.0f,
         0.0f, -h/2,  halfDepth, 0,0,0, u0, 0.0f,

        // Side 2 (left slant edge)
        -halfBase,  h/2, -halfDepth, 0,0,0, u1, 1.0f,
         0.0f,  h/2,  halfDepth,    0,0,0, u2, 1.0f,
         0.0f, -h/2,  halfDepth,    0,0,0, u2, 0.0f,
        -halfBase, -h/2, -halfDepth, 0,0,0, u1, 0.0f,

        // Side 3 (base edge)
         halfBase,  h/2, -halfDepth, 0,0,0, u2, 1.0f,
        -halfBase, h/2, -halfDepth, 0,0,0, u3, 1.0f,
        -halfBase, -h/2, -halfDepth,0,0,0, u3, 0.0f,
         halfBase, -h/2, -halfDepth,0,0,0, u2, 0.0f
    };

    std::vector<unsigned int> indices = {
        // top & bottom
        0,1,2,
        3,5,4,
        // sides
        6,7,8, 6,8,9,
        10,11,12, 10,12,13,
        14,16,17, 14,15,16
    };

    // Compute normals dynamically per triangle
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int ia = indices[i];
        unsigned int ib = indices[i+1];
        unsigned int ic = indices[i+2];

        glm::vec3 v0(localVertices[ia*8+0], localVertices[ia*8+1], localVertices[ia*8+2]);
        glm::vec3 v1(localVertices[ib*8+0], localVertices[ib*8+1], localVertices[ib*8+2]);
        glm::vec3 v2(localVertices[ic*8+0], localVertices[ic*8+1], localVertices[ic*8+2]);

        glm::vec3 normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));

        for (int j=0;j<3;++j){
            unsigned int idx = indices[i+j];
            localVertices[idx*8+3] = normal.x;
            localVertices[idx*8+4] = normal.y;
            localVertices[idx*8+5] = normal.z;
        }
    }

    setupVAO(localVertices, indices, 8);
}





// ---------------- Texture loading ----------------
void Primitive::loadTexture(const std::string& path) {
    // Resolve absolute path relative to current directory
    fs::path texPath = ResolveAssetPath(path, "textures");
    std::cout << "[DEBUG] Attempting to load texture: " << texPath << std::endl;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (!fs::exists(texPath)) {
        std::cerr << "[ERROR] Texture file not found: " << texPath << std::endl;

        // Create a simple 2x2 checkerboard fallback texture
        unsigned char checker[16] = {
            255,255,255, 0,0,0,
            0,0,0,       255,255,255
        };

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, checker);
        glGenerateMipmap(GL_TEXTURE_2D);
        return;
    }

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texPath.string().c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 1) ? GL_RED : (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "[DEBUG] Loaded texture successfully: " << texPath << std::endl;
    } else {
        std::cerr << "[ERROR] stbi_load failed for: " << texPath
                  << " (file not found or unsupported format)" << std::endl;

        // Fallback 2x2 checkerboard
        unsigned char checker[16] = {
            255,255,255, 0,0,0,
            0,0,0,       255,255,255
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, checker);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(data);
}

void Primitive::constructShapeMesh() {
    switch (type) {
        case PrimitiveType::Cube:            setupCube(); break;
        case PrimitiveType::TriangularPrism: setupTriangularPrism(); break;
        case PrimitiveType::Sphere:          setupSphere(); break;
        case PrimitiveType::Triangle:        setupTriangle(); break;
        case PrimitiveType::Plane:           setupPlane(); break;
        case PrimitiveType::Slab:            setupSlab(); break;
        default: break;
    }

}



// ---------------- Draw ----------------
void Primitive::draw() const {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    //glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Primitive::drawWireframe() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

namespace {
glm::vec3 ClosestPointOnTriangle(const glm::vec3& point, const glm::vec3& a,
                                 const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = point - a;
    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    const glm::vec3 bp = point - b;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        return a + (d1 / (d1 - d3)) * ab;

    const glm::vec3 cp = point - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        return a + (d2 / (d2 - d6)) * ac;

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        return b + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (c - b);

    const float denominator = 1.0f / (va + vb + vc);
    return a + ab * (vb * denominator) + ac * (vc * denominator);
}
}

bool Primitive::IntersectsSphere(const glm::vec3& center, float radius,
                                 glm::vec3* contactNormal) const {
    const float radiusSquared = radius * radius;
    float closestDistanceSquared = radiusSquared;
    bool intersects = false;
    for (size_t i = 0; i + 2 < localIndices.size(); i += 3) {
        glm::vec3 triangle[3];
        for (int vertex = 0; vertex < 3; ++vertex) {
            const size_t offset = static_cast<size_t>(localIndices[i + vertex]) * 8;
            triangle[vertex] = glm::vec3(modelMatrix * glm::vec4(
                localVertices[offset], localVertices[offset + 1], localVertices[offset + 2], 1.0f));
        }
        const glm::vec3 closest = ClosestPointOnTriangle(center, triangle[0], triangle[1], triangle[2]);
        const glm::vec3 separation = center - closest;
        const float distanceSquared = glm::dot(separation, separation);
        if (distanceSquared <= radiusSquared && (!intersects || distanceSquared < closestDistanceSquared)) {
            intersects = true;
            closestDistanceSquared = distanceSquared;

            if (contactNormal) {
                if (distanceSquared > 0.0000001f) {
                    *contactNormal = separation / glm::sqrt(distanceSquared);
                } else {
                    const glm::vec3 triangleNormal = glm::cross(
                        triangle[1] - triangle[0], triangle[2] - triangle[0]);
                    const float normalLengthSquared = glm::dot(triangleNormal, triangleNormal);
                    *contactNormal = normalLengthSquared > 0.0000001f
                        ? triangleNormal / glm::sqrt(normalLengthSquared)
                        : glm::vec3(0.0f);
                }
            }
        }
    }
    return intersects;
}
