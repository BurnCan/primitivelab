#include "SceneManager.h"
#include "Camera.h"
#include "Terrain.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>


//Terrain
SceneManager::SceneManager()
    : terrain(20, 20, 1.0f) // width, depth, scale for terrain
{
    // Optionally set a default texture for terrain
    terrain.SetTexturePath("../textures/grass.jpg");

}

// Persistent light sphere
//SceneManager::SceneManager()
    //: lightSphere(PrimitiveType::Sphere, "sun.jpg", 16, 16) {}

bool SceneManager::loadScene(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open scene file: " << path << std::endl;
        return false;
    }

    primitives.clear();
    lights.clear();

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments or empty lines
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string primTypeStr, texturePath;
        float px, py, pz, rx, ry, rz, sx, sy, sz;

        if (!(iss >> primTypeStr)) continue;

        if (primTypeStr == "Cube" || primTypeStr == "TriangularPrism" || primTypeStr == "Sphere" || primTypeStr == "Plane" || primTypeStr == "Slab" || primTypeStr == "Triangle") {
            if (!(iss >> texturePath >> px >> py >> pz >> rx >> ry >> rz >> sx >> sy >> sz)) continue;

            PrimitiveType type;
            if (primTypeStr == "Cube") type = PrimitiveType::Cube;
            else if (primTypeStr == "TriangularPrism") type = PrimitiveType::TriangularPrism;
            else if (primTypeStr == "Sphere") type = PrimitiveType::Sphere;
            else if (primTypeStr == "Plane") type = PrimitiveType::Plane;
            else if (primTypeStr == "Slab") type = PrimitiveType::Slab;
            else type = PrimitiveType::Triangle;

            auto prim = std::make_unique<Primitive>(type, texturePath);
            prim->position = glm::vec3(px, py, pz);
            prim->rotation = glm::vec3(rx, ry, rz);
            prim->scale    = glm::vec3(sx, sy, sz);
            prim->UpdateModelMatrix();

            primitives.push_back(std::move(prim));
        }
        else if (primTypeStr == "Light") {
            Light light;
            float r, g, b;
            if (!(iss >> px >> py >> pz >> r >> g >> b)) continue;
            light.position = glm::vec3(px, py, pz);
            light.color    = glm::vec3(r, g, b);
            lights.push_back(light);
        }
        UpdateBoundingBoxes();
    }

    return true;
}

bool SceneManager::saveScene(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open scene file for writing: " << path << std::endl;
        return false;
    }

    // Comment header explaining format
    file << "# Scene file format:\n";
    file << "# PrimitiveType TexturePath posX posY posZ rotX rotY rotZ scaleX scaleY scaleZ\n";
    file << "# Light posX posY posZ colorR colorG colorB\n";

    // Save primitives
    for (const auto& prim : primitives) {
        std::string typeName = prim->GetTypeName();

        //remove any spaces from typename to avoid conflict with scene file format
        //while preserving readability for the UI (see GetTypeName in Primitives.h )
        typeName.erase(std::remove(typeName.begin(), typeName.end(), ' '), typeName.end());

        file << typeName << " "
             << prim->GetTexturePath() << " "
             << prim->position.x << " " << prim->position.y << " " << prim->position.z << " "
             << prim->rotation.x << " " << prim->rotation.y << " " << prim->rotation.z << " "
             << prim->scale.x << " " << prim->scale.y << " " << prim->scale.z << "\n";
    }

    // Save lights
    for (const auto& light : lights) {
        file << "Light "
             << light.position.x << " " << light.position.y << " " << light.position.z << " "
             << light.color.r << " " << light.color.g << " " << light.color.b << "\n";
    }

    return true;
}



void SceneManager::drawScene(const Shader& shader, Camera& camera, int width, int height)
{
    // --- Ensure OpenGL state ---
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    //glFrontFace(GL_CCW); // Assuming your vertices are CCW
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- Use shader ---
    shader.Use();

    // --- View and Projection from camera ---
    glm::mat4 view       = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix(width, height);

    shader.SetMat4("view", view);
    shader.SetMat4("projection", projection);
    shader.SetVec3("viewPos", camera.Position);

    // --- Set lights ---
    int count = std::min((int)lights.size(), 10); // max 10 lights
    shader.SetInt("numLights", count);
    for (int i = 0; i < count; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        shader.SetVec3(base + ".position", lights[i].position);
        shader.SetVec3(base + ".color", lights[i].color);
        shader.SetFloat(base + ".shininess", 32.0f);
    }

    // --- Draw primitives ---
    for (auto& prim : primitives) {
        shader.SetInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, prim->GetTextureID());

        // Make sure model matrix is updated correctly
        prim->UpdateModelMatrix();
        shader.SetMat4("model", prim->modelMatrix);

        prim->draw();
    }

    // --- Draw terrain ---
    shader.SetInt("texture1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrain.GetTextureID());

    glm::mat4 terrainModel = glm::mat4(1.0f); // identity
    shader.SetMat4("model", terrainModel);

    terrain.Draw(shader.ID);
}





void SceneManager::UpdateBoundingBoxes() {
    boundingBoxes.clear();

    // Add terrain AABB (flat plane)
    AABB terrainBox;
    terrainBox.min = glm::vec3(-10.0f, -0.1f, -10.0f);
    terrainBox.max = glm::vec3(10.0f, 0.1f, 10.0f);
    boundingBoxes.push_back(terrainBox);

    // Add primitives
    for (const auto& prim : primitives) {
        glm::vec3 halfScale = prim->scale * 0.5f;
        glm::vec3 min = prim->position - halfScale;
        glm::vec3 max = prim->position + halfScale;
        boundingBoxes.push_back({ min, max });
    }
}

bool SceneManager::CheckCollision(const glm::vec3& point, float radius) {
    for (const auto& box : boundingBoxes) {
        if (box.IntersectsSphere(point, radius)) {
            return true;
        }
    }
    return false;
}





void SceneManager::drawLights(const Shader& lightShader, Camera& camera, int width, int height)
{
    lightShader.Use();  // make sure the shader is active

    // ----- Matrices -----
    glm::mat4 view       = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix(width, height); // use Camera helper

    lightShader.SetMat4("view", view);
    lightShader.SetMat4("projection", projection);

    // ----- Draw each light as a small sphere -----
    for (const auto& light : lights) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
        model = glm::scale(model, glm::vec3(0.1f)); // tiny sphere
        lightShader.SetMat4("model", model);
        lightShader.SetVec3("lightColor", light.color);

        lightSphere.draw(); // your mesh for a sphere
    }
}


void SceneManager::DrawBoundingBoxes(const Shader& shader, const Camera& camera, int width, int height)
{
    // Backup minimal GL state we change
    GLint polygonMode[2];
glGetIntegerv(GL_POLYGON_MODE, polygonMode);
std::cout << "POLYMODE START: " << polygonMode[0] << std::endl;
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLint prevArrayBuffer = 0; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    GLint prevElementArray = 0; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementArray);

    // Use shader and upload camera matrices
    Shader& mutableShader = const_cast<Shader&>(shader);
    mutableShader.Use();
    mutableShader.SetMat4("view", camera.GetViewMatrix());

    // If Camera doesn't provide a projection helper, compute here:
    //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    //mutableShader.SetMat4("projection", projection);
    mutableShader.SetMat4("projection", camera.GetProjectionMatrix(width, height));



    // green color for boxes
    mutableShader.SetVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));

    // draw in wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Keep depth testing ON, but ensure we do NOT write to depth buffer
    if (!depthTestEnabled) glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // don't write depth so boxes don't occlude later draws

    // Optionally enable line smoothing if desired (not necessary)
    // glEnable(GL_LINE_SMOOTH);

    for (const auto& primPtr : primitives)
    {
        const Primitive& prim = *primPtr;
        const glm::vec3& min = prim.boundingBox.min;
        const glm::vec3& max = prim.boundingBox.max;

        // Model is primitive's modelMatrix (updated by UpdateModelMatrix)
        glm::mat4 model = prim.modelMatrix;
        mutableShader.SetMat4("model", model);

        // Build 8 corners in local space
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {max.x, max.y, min.z}, {min.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z}
        };

        // Indices for 12 edges (24 index entries)
        const unsigned int indices[24] = {
            0,1, 1,2, 2,3, 3,0, // bottom
            4,5, 5,6, 6,7, 7,4, // top
            0,4, 1,5, 2,6, 3,7  // sides
        };

        // Upload corner positions to a tiny temporary VBO (cheap)
        GLuint tmpVAO=0, tmpVBO=0, tmpEBO=0;
        glGenVertexArrays(1, &tmpVAO);
        glGenBuffers(1, &tmpVBO);
        glGenBuffers(1, &tmpEBO);

        glBindVertexArray(tmpVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmpEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

        // cleanup
        glBindVertexArray(0);
        glDeleteBuffers(1, &tmpVBO);
        glDeleteBuffers(1, &tmpEBO);
        glDeleteVertexArrays(1, &tmpVAO);
    }

    // restore depth mask and polygon mode and bindings
    glDepthMask(GL_TRUE);
    if (!depthTestEnabled) glDisable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);

    // restore bindings
    glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementArray);

    if (!cullFaceEnabled) glDisable(GL_CULL_FACE);

    glUseProgram(0);

    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
std::cout << "POLYMODE END: " << polygonMode[0] << std::endl;

}







void SceneManager::InitDebugCube()
{
    if (debugVAO != 0) return;

    float vertices[] = {
        // 12 edges * 2 points = 24 vertices
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f
    };

    GLuint VBO;
    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}




