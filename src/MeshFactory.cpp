#include "TextureUtils.h"
#include "MeshObject.h"
#include "Terrain.h"
#include <type_traits>
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

namespace MeshGeneration {
MeshData CreateTriangle();
MeshData CreatePlane();
MeshData CreateCube();
MeshData CreateSlab();
MeshData CreateTriangularPrism();
MeshData CreateSphere(unsigned int, unsigned int);
MeshData CreateRock();
}

namespace {
MeshData TypedMeshData(const std::vector<float>& values, std::vector<unsigned int> indices) {
 MeshData data; data.vertices.reserve(values.size()/8);
 for(std::size_t i=0;i+7<values.size();i+=8) data.vertices.push_back({{values[i],values[i+1],values[i+2]},{values[i+3],values[i+4],values[i+5]},{values[i+6],values[i+7]}});
 data.indices=std::move(indices); return data;
}
}

// ---------------- Rock setup ----------------
MeshData MeshGeneration::CreateRock() {
    std::vector<float> localVertices = {
        // positions              // normals          // texcoords

        // Base
    -0.467770815f, -0.477016717f, -0.38251707f, -0.548177242f, -0.747286081f, 0.375586629f, 0.0f, 0.0f,
    0.405647159f, -0.518806696f, -0.370780468f, -0.487694502f, -0.80298835f, 0.342584163f, 0.933747292f, 0.0142049817f,
    0.203193307f, 0.091897577f, -0.0833028257f, -0.425442696f, -0.185806394f, 0.885705709f, 0.717309415f, 0.362143397f,
    -0.12446475f, 0.123544455f, -0.266472578f, -0.613453865f, 0.775359392f, 0.149973854f, 0.367019117f, 0.140450358f,
    -0.463334203f, -0.496991307f, 0.379545957f, -0.437826157f, -0.437254936f, 0.785567462f, 0.00474306056f, 0.922336102f,
    0.4676193f, -0.496454954f, 0.443714261f, -0.329319268f, -0.874644995f, 0.355731517f, 1.0f, 1.0f,
    0.117086768f, 0.134675384f, 0.28392005f, -0.675754726f, 0.170806259f, 0.717064023f, 0.625255227f, 0.806598723f,
    -0.303527951f, 0.0646477044f, 0.32624054f, -0.79199934f, 0.591188431f, 0.152424723f, 0.17558755f, 0.857819796f

        // Add separately duplicated vertices for each side.
        // Each side needs its own face normal and UV coordinates.
    };

    std::vector<unsigned int> indices = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4,
    0, 4, 7,
    7, 3, 0,
    1, 5, 6,
    6, 2, 1,
    3, 7, 6,
    6, 2, 3,
    0, 1, 5,
    5, 4, 0


        // Add side triangles here.
    };

    return TypedMeshData(localVertices, std::move(indices));
}


// ---------------- Triangle setup ----------------
MeshData MeshGeneration::CreateTriangle() {
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
MeshData MeshGeneration::CreatePlane() {
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
MeshData MeshGeneration::CreateSlab() {
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
MeshData MeshGeneration::CreateCube() {
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
MeshData MeshGeneration::CreateSphere (unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS) {
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




MeshData MeshGeneration::CreateTriangularPrism() {
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





MeshSource MeshSources::Triangle() { return TriangleMeshSource{}; }
MeshSource MeshSources::Plane() { return PlaneMeshSource{}; }
MeshSource MeshSources::Cube() { return CubeMeshSource{}; }
MeshSource MeshSources::Slab() { return SlabMeshSource{}; }
MeshSource MeshSources::TriangularPrism() { return TriangularPrismMeshSource{}; }
MeshSource MeshSources::Sphere(unsigned int x, unsigned int y) { return SphereMeshSource{x, y}; }
MeshSource MeshSources::Terrain(const TerrainMeshSource& source) { return source; }
MeshSource MeshSources::Rock() { return RockMeshSource{};}

MeshData MeshFactory::Create(const MeshSource& source) {
    return std::visit([](const auto& definition) -> MeshData {
        using T = std::decay_t<decltype(definition)>;
        if constexpr (std::is_same_v<T, TriangleMeshSource>) return MeshGeneration::CreateTriangle();
        else if constexpr (std::is_same_v<T, PlaneMeshSource>) return MeshGeneration::CreatePlane();
        else if constexpr (std::is_same_v<T, CubeMeshSource>) return MeshGeneration::CreateCube();
        else if constexpr (std::is_same_v<T, SlabMeshSource>) return MeshGeneration::CreateSlab();
        else if constexpr (std::is_same_v<T, TriangularPrismMeshSource>) return MeshGeneration::CreateTriangularPrism();
        else if constexpr (std::is_same_v<T, SphereMeshSource>) return MeshGeneration::CreateSphere(definition.xSegments, definition.ySegments);
        else if constexpr (std::is_same_v<T, RockMeshSource>) return MeshGeneration::CreateRock();
        else return TerrainGeometry::CreateMeshData(definition);
    }, source);
}

MeshObject::MeshObject(MeshSource definition, const std::string& path)
    : source(std::move(definition)), geometry(MeshFactory::Create(source)), mesh(geometry) {
    SetTexturePath(path);
}
MeshObject::~MeshObject() { if (textureID) glDeleteTextures(1, &textureID); }
MeshObject::MeshObject(MeshObject&& other) noexcept
    : modelMatrix(other.modelMatrix), source(std::move(other.source)), geometry(std::move(other.geometry)),
      mesh(std::move(other.mesh)), texturePath(std::move(other.texturePath)), textureID(other.textureID) {
    other.textureID = 0;
}
MeshObject& MeshObject::operator=(MeshObject&& other) noexcept {
    if (this == &other) return *this;
    if (textureID) glDeleteTextures(1, &textureID);
    modelMatrix = other.modelMatrix; source = std::move(other.source); geometry = std::move(other.geometry);
    mesh = std::move(other.mesh); texturePath = std::move(other.texturePath); textureID = other.textureID;
    other.textureID = 0; return *this;
}
void MeshObject::Draw() const { mesh.Draw(); }
void MeshObject::DrawWireframe() const { mesh.Draw(); }
bool MeshObject::IntersectsSphere(const glm::vec3& center, float radius, glm::vec3* normal) const {
    return IntersectsSphereMesh(center, radius, geometry, modelMatrix, normal);
}
void MeshObject::SetTexturePath(const std::string& path) {
    texturePath = fs::path(path).filename().string();
    ReloadTexture();
}
void MeshObject::ReloadTexture() {
    if (textureID) glDeleteTextures(1, &textureID);
    textureID = 0;
    if (!texturePath.empty()) LoadTexture();
}
void MeshObject::LoadTexture() {
    const fs::path path = ResolveAssetPath(texturePath, "textures");
    glGenTextures(1, &textureID); glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, channels; stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!pixels) { std::cerr << "Failed to load texture: " << path << '\n'; return; }
    const GLenum format = channels == 1 ? GL_RED : channels == 3 ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D); stbi_image_free(pixels);
}
std::string MeshObject::GetSourceName() const {
    return std::visit([](const auto& definition) -> std::string {
        using T = std::decay_t<decltype(definition)>;
        if constexpr (std::is_same_v<T, TriangleMeshSource>) return "Triangle";
        else if constexpr (std::is_same_v<T, PlaneMeshSource>) return "Plane";
        else if constexpr (std::is_same_v<T, CubeMeshSource>) return "Cube";
        else if constexpr (std::is_same_v<T, SlabMeshSource>) return "Slab";
        else if constexpr (std::is_same_v<T, TriangularPrismMeshSource>) return "Triangular Prism";
        else if constexpr (std::is_same_v<T, SphereMeshSource>) return "Sphere";
        else if constexpr (std::is_same_v<T, RockMeshSource>) return "Rock";
        else return "Terrain";
    }, source);
}
