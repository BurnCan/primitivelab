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





bool SceneManager::CheckCollision(const glm::vec3& point, float radius,
                                  glm::vec3* contactNormal) {
    if (terrain.IntersectsSphere(point, radius, contactNormal)) return true;

    for (const auto& primitive : primitives) {
        primitive->UpdateModelMatrix();
        if (primitive->IntersectsSphere(point, radius, contactNormal)) return true;
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


void SceneManager::DrawCollisionMeshes(const Shader& shader, const Camera& camera, int width, int height,
                                       bool drawPrimitiveMeshes, bool drawTerrainMesh)
{
    // Backup minimal GL state we change
    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
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

    if (drawPrimitiveMeshes) {
        for (const auto& primPtr : primitives)
        {
            const Primitive& prim = *primPtr;
            const_cast<Primitive&>(prim).UpdateModelMatrix();
            mutableShader.SetMat4("model", prim.modelMatrix);
            prim.drawWireframe();
        }
    }

    if (drawTerrainMesh) {
        mutableShader.SetVec3("color", glm::vec3(1.0f, 0.65f, 0.0f));
        mutableShader.SetMat4("model", glm::mat4(1.0f));
        terrain.DrawWireframe();
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

}
