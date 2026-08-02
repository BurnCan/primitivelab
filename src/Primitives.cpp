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

void detail::PrimitiveMesh::SetTexturePath(const std::string& newPath) {
    // Normalize to filename only
    fs::path cleanPath = fs::path(newPath).filename();
    texturePath = cleanPath.string();
    ReloadTexture();
}

void detail::PrimitiveMesh::ReloadTexture() {
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
detail::PrimitiveMesh::PrimitiveMesh(detail::PrimitiveMeshType type, const std::string& texturePath)
    : type(type)
{
    // Normalize: always use only the filename
    fs::path cleanPath = fs::path(texturePath).filename();
    this->texturePath = cleanPath.string();

    geometry = PrimitiveGeometry::Create(type, X_SEGMENTS, Y_SEGMENTS);
    mesh.Upload(geometry);
    loadTexture(this->texturePath);
}

detail::PrimitiveMesh::PrimitiveMesh(detail::PrimitiveMeshType type, const std::string& texturePath, unsigned int xSeg, unsigned int ySeg)
    : type(type), X_SEGMENTS(xSeg), Y_SEGMENTS(ySeg)
{
    // Normalize: always use only the filename
    fs::path cleanPath = fs::path(texturePath).filename();
    this->texturePath = cleanPath.string();

    geometry = PrimitiveGeometry::Create(type, X_SEGMENTS, Y_SEGMENTS);
    mesh.Upload(geometry);
    loadTexture(this->texturePath);
}


// ---------------- Destructor ----------------
detail::PrimitiveMesh::~PrimitiveMesh() {
    if (textureID) glDeleteTextures(1, &textureID);
}


// ---------------- Mesh Construction Dispatcher ----------------
// Selects the appropriate setup function to generate vertex/index buffers
// and configure the VAO/VBO/EBO for this primitive type.
//void detail::PrimitiveMesh::constructShapeMesh() {
//    switch (type) {
//        case detail::PrimitiveMeshType::Cube:
//            setupCube();
//            std::cout << "[DEBUG] Constructed Cube: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//        case detail::PrimitiveMeshType::Sphere:
//            setupSphere();
//            std::cout << "[DEBUG] Constructed Sphere: VAO=" << VAO
//                      << ", indexCount=" << indexCount
//                      << ", Segments=" << X_SEGMENTS << "x" << Y_SEGMENTS << std::endl;
//            break;
//        case detail::PrimitiveMeshType::TriangularPrism:
//            setupTriangularPrism();
//            std::cout << "[DEBUG] Constructed TriangularPrism: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//        case detail::PrimitiveMeshType::Triangle:
 //           setupTriangle();
//            std::cout << "[DEBUG] Constructed Triangle: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//            case detail::PrimitiveMeshType::Plane:
//            setupPlane();
//            std::cout << "[DEBUG] Constructed Plane: VAO=" << VAO
//                      << ", indexCount=" << indexCount << std::endl;
//            break;
//            case detail::PrimitiveMeshType::Slab:
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

namespace {
MeshData TypedMeshData(const std::vector<float>& values, std::vector<unsigned int> indices) {
 MeshData data; data.vertices.reserve(values.size()/8);
 for(std::size_t i=0;i+7<values.size();i+=8) data.vertices.push_back({{values[i],values[i+1],values[i+2]},{values[i+3],values[i+4],values[i+5]},{values[i+6],values[i+7]}});
 data.indices=std::move(indices); return data;
}
}

// ---------------- Triangle setup ----------------
MeshData detail::PrimitiveGeometry::CreateTriangle() {
    //std::vector<float> vertices = {
    std::vector<float> localVertices = {
        // positions       // normals       // texcoords
         0.0f,  0.5f, 0.0f, 0,0,1, 0.5f,1.0f, // top
        -0.5f, -0.5f, 0.0f, 0,0,1, 0.0f,0.0f, // bottom left
         0.5f, -0.5f, 0.0f, 0,0,1, 1.0f,0.0f  // bottom right
    };

    std::vector<unsigned int> indices = { 0, 1, 2 };

    return TypedMeshData(localVertices, std::move(indices));
}

// ---------------- Plane setup ----------------
MeshData detail::PrimitiveGeometry::CreatePlane() {
    float h = 0.5f; // half size, makes a 1x1 unit plane

    //std::vector<float> vertices = {
    std::vector<float> localVertices = {
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

    return TypedMeshData(localVertices, std::move(indices));
}

// ---------------- Slab setup ----------------
MeshData detail::PrimitiveGeometry::CreateSlab() {
    float width  = 1.0f;
    float depth  = 1.0f;
    float height = 0.1f;

    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    float hh = height * 0.5f;

    //std::vector<float> vertices = {
    std::vector<float> localVertices = {
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

    return TypedMeshData(localVertices, std::move(indices));
}



// ---------------- Cube setup ----------------
MeshData detail::PrimitiveGeometry::CreateCube() {
    //std::vector<float> vertices = {
    std::vector<float> localVertices = {
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

    return TypedMeshData(localVertices, std::move(indices));
}

// ---------------- Sphere setup ----------------
MeshData detail::PrimitiveGeometry::CreateSphere (unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS) {
    std::vector<float> localVertices;
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

    return TypedMeshData(localVertices, std::move(indices));
}




MeshData detail::PrimitiveGeometry::CreateTriangularPrism() {
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
    std::vector<float> localVertices = {
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

    return TypedMeshData(localVertices, std::move(indices));
}





// ---------------- Texture loading ----------------
void detail::PrimitiveMesh::loadTexture(const std::string& path) {
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

MeshData detail::PrimitiveGeometry::Create(PrimitiveMeshType type, unsigned int xSegments, unsigned int ySegments) {
 switch(type) {
 case PrimitiveMeshType::Cube:return CreateCube(); case PrimitiveMeshType::TriangularPrism:return CreateTriangularPrism();
 case PrimitiveMeshType::Sphere:return CreateSphere(xSegments,ySegments); case PrimitiveMeshType::Triangle:return CreateTriangle();
 case PrimitiveMeshType::Plane:return CreatePlane(); case PrimitiveMeshType::Slab:return CreateSlab(); } return {};
}

// ---------------- Draw ----------------
void detail::PrimitiveMesh::draw() const {
    glBindTexture(GL_TEXTURE_2D, textureID);
    mesh.Draw();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void detail::PrimitiveMesh::drawWireframe() const {
    mesh.Draw();
}

bool detail::PrimitiveMesh::IntersectsSphere(const glm::vec3& center, float radius, glm::vec3* contactNormal) const {
 return IntersectsSphereMesh(center, radius, geometry, modelMatrix, contactNormal);
}

std::string detail::PrimitiveMesh::GetTypeName() const { switch(type) {
 case PrimitiveMeshType::Cube:return "Cube"; case PrimitiveMeshType::TriangularPrism:return "Triangular Prism"; case PrimitiveMeshType::Sphere:return "Sphere";
 case PrimitiveMeshType::Triangle:return "Triangle"; case PrimitiveMeshType::Plane:return "Plane"; case PrimitiveMeshType::Slab:return "Slab";} return "Unknown"; }
