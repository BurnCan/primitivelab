// ---------------- Terrain.cpp ----------------
#include "Terrain.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stb_image.h>
#include <filesystem>
namespace fs = std::filesystem;

Terrain::Terrain(int width, int depth, float scale)
{
    VAO = VBO = EBO = 0;
    textureID = 0;
    indexCount = 0;

    GenerateMesh(width, depth, scale);
}

void Terrain::GenerateMesh(int width, int depth, float scale)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    // Create a grid of quads (unit squares)
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            // positions
            float xpos = (x - halfW) * scale;
            float zpos = (z - halfD) * scale;
            vertices.push_back(xpos);
            vertices.push_back(0.0f); // flat plane
            vertices.push_back(zpos);

            // normals (up)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);

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

void Terrain::Draw(GLuint shaderProgram)
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

    fs::path texPath = fs::current_path().parent_path() / "textures" / path;
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
