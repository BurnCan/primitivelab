// ---------------- Terrain.cpp ----------------
#include "Terrain.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stb_image.h>
#include <filesystem>
#include "AssetPaths.h"
namespace fs = std::filesystem;

Terrain::Terrain(const TerrainConfig& value)
    : config(value), textureID(0), geometry(TerrainGeometry::CreateMeshData(config)), mesh(geometry)
{
}
Terrain::Terrain(int width, int depth, float scale) : Terrain(TerrainConfig{TerrainGeometryType::Flat, TerrainPlane::XZ, width, depth, width * scale, depth * scale, 1.0f, {}}) {}
Terrain::~Terrain() {
    if (textureID) glDeleteTextures(1, &textureID);
}

MeshData TerrainGeometry::CreateMeshData(const TerrainConfig& config)
{
    const int width = config.widthSegments, depth = config.depthSegments;
    MeshData data;

    // Create a grid of quads (unit squares)
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            // positions
            float u = (float(x) / width - 0.5f) * config.width;
            float v = (float(z) / depth - 0.5f) * config.depth;
            glm::vec3 p, normal;
            if (config.plane == TerrainPlane::XY) { p={u,v,0}; normal={0,0,1}; }
            else if (config.plane == TerrainPlane::YZ) { p={0,u,v}; normal={1,0,0}; }
            else { p={u,0,v}; normal={0,1,0}; }
            data.vertices.push_back({p, normal, {static_cast<float>(x), static_cast<float>(z)}});
        }
    }

    // Indices
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            int start = z * (width + 1) + x;

            data.indices.insert(data.indices.end(), {static_cast<unsigned int>(start),
                static_cast<unsigned int>(start + width + 1), static_cast<unsigned int>(start + 1),
                static_cast<unsigned int>(start + 1), static_cast<unsigned int>(start + width + 1),
                static_cast<unsigned int>(start + width + 2)});
        }
    }

    return data;
}

void Terrain::DrawWireframe() const
{
    mesh.Draw();
}

bool Terrain::IntersectsSphere(const glm::vec3& center, float radius,
                               glm::vec3* contactNormal, const glm::mat4& model) const
{
    return IntersectsSphereMesh(center, radius, geometry, model, contactNormal);
}

void Terrain::Draw()
{
    mesh.Draw();
}

void Terrain::SetTexturePath(const std::string& path)
{
    texturePath = path;

    if (textureID != 0)
        glDeleteTextures(1, &textureID);

    fs::path texPath = ResolveAssetPath(path, "textures");
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, nc;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texPath.string().c_str(), &w, &h, &nc, 0);
    if (data) {
        GLenum format = (nc == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load terrain texture: " << texPath << std::endl;
        stbi_image_free(data);
    }
}
