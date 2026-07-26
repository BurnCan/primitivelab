#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

// --- Forward declarations ---
std::string LoadShaderSource(const std::string& path);
GLuint CompileShader(GLenum type, const std::string& source, const std::string& label);
GLuint CompileShaderProgram(const std::string& vertPath, const std::string& fragPath);

class Shader {
public:
    GLuint ID;

    Shader() : ID(0) {}

    Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertSource = LoadShaderSource(vertexPath);
        std::string fragSource = LoadShaderSource(fragmentPath);
        GLuint vert = CompileShader(GL_VERTEX_SHADER, vertSource, vertexPath);
        GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragSource, fragmentPath);

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);

        // --- Linking error check ---
        GLint success;
        char infoLog[1024];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINK_FAILED\n"
                      << "Vertex: " << vertexPath << "\nFragment: " << fragmentPath
                      << "\n" << infoLog << std::endl;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    // --- Use program ---
void use() const { glUseProgram(ID); }
void Use() const { use(); } // ✅ Compatibility alias (uppercase version)


    // --- Uniform helpers ---
    void SetMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void SetVec3(const std::string& name, const glm::vec3& vec) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
    }

    void SetFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void SetInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
};
