// ---------------- Terrain.cpp ----------------
#include "Terrain.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stb_image.h>
#include <filesystem>
#include <limits>
#include "AssetPaths.h"
namespace fs = std::filesystem;

namespace {
glm::vec3 ClosestPointOnTriangle(const glm::vec3& point, const glm::vec3& a,
                                 const glm::vec3& b, const glm::vec3& c)
{
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
        return a + ab * (d1 / (d1 - d3));

    const glm::vec3 cp = point - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        return a + ac * (d2 / (d2 - d6));

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

    const float denominator = 1.0f / (va + vb + vc);
    return a + ab * (vb * denominator) + ac * (vc * denominator);
}
}

Terrain::Terrain(const TerrainConfig& value) : config(value)
{
    VAO = VBO = EBO = 0;
    textureID = 0;
    indexCount = 0;

    GenerateMesh();
}
Terrain::Terrain(int width, int depth, float scale) : Terrain(TerrainConfig{TerrainGeometryType::Flat, TerrainPlane::XZ, width, depth, width * scale, depth * scale, 1.0f, {}}) {}
Terrain::~Terrain() {
    if (textureID) glDeleteTextures(1, &textureID);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}

void Terrain::GenerateMesh()
{
    const int width = config.widthSegments, depth = config.depthSegments;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    collisionVertices.clear();
    collisionIndices.clear();

    float halfW = config.width * 0.5f;
    float halfD = config.depth * 0.5f;

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
            vertices.insert(vertices.end(), {p.x,p.y,p.z});
            collisionVertices.push_back(p);

            // normals (up)
            vertices.insert(vertices.end(), {normal.x,normal.y,normal.z});

            // texcoords (repeat per unit square)
            vertices.push_back((float)x); // we’ll use integer coords for tiling
            vertices.push_back((float)z);
        }
    }

    // Indices
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            int start = z * (width + 1) + x;

            indices.push_back(start);
            indices.push_back(start + width + 1);
            indices.push_back(start + 1);

            indices.push_back(start + 1);
            indices.push_back(start + width + 1);
            indices.push_back(start + width + 2);
        }
    }

    indexCount = static_cast<int>(indices.size());
    collisionIndices = indices;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texcoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Terrain::DrawWireframe() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

bool Terrain::IntersectsSphere(const glm::vec3& center, float radius,
                               glm::vec3* contactNormal, const glm::mat4& model) const
{
    const float radiusSquared = radius * radius;
    float closestDistanceSquared = std::numeric_limits<float>::max();
    glm::vec3 bestNormal(0.0f, 1.0f, 0.0f);
    bool intersects = false;

    for (size_t i = 0; i + 2 < collisionIndices.size(); i += 3) {
        const glm::vec3 a = glm::vec3(model * glm::vec4(collisionVertices[collisionIndices[i]], 1.0f));
        const glm::vec3 b = glm::vec3(model * glm::vec4(collisionVertices[collisionIndices[i + 1]], 1.0f));
        const glm::vec3 c = glm::vec3(model * glm::vec4(collisionVertices[collisionIndices[i + 2]], 1.0f));
        const glm::vec3 closest = ClosestPointOnTriangle(center, a, b, c);
        const glm::vec3 separation = center - closest;
        const float distanceSquared = glm::dot(separation, separation);

        if (distanceSquared <= radiusSquared && distanceSquared < closestDistanceSquared) {
            intersects = true;
            closestDistanceSquared = distanceSquared;
            if (distanceSquared > 0.0000001f) {
                bestNormal = separation / glm::sqrt(distanceSquared);
            } else {
                const glm::vec3 faceNormal = glm::cross(b - a, c - a);
                if (glm::dot(faceNormal, faceNormal) > 0.0000001f)
                    bestNormal = glm::normalize(faceNormal);
            }
        }
    }

    if (intersects && contactNormal) *contactNormal = bestNormal;
    return intersects;
}

void Terrain::Draw()
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
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
