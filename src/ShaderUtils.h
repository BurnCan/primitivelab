#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

std::string LoadShaderSource(const std::string& path);
GLuint CompileShader(GLenum type, const std::string& source, const std::string& label);
GLuint CompileShaderProgram(const std::string& vertPath, const std::string& fragPath);

class Shader {
public:
    GLuint ID = 0;
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    bool IsValid() const { return ID != 0; }
    const std::string& GetLastError() const { return lastError; }
    void use() const { glUseProgram(ID); }
    void Use() const { use(); }
    void SetMat4(const std::string&, const glm::mat4&) const;
    void SetVec3(const std::string&, const glm::vec3&) const;
    void SetFloat(const std::string&, float) const;
    void SetInt(const std::string&, int) const;
private:
    std::string lastError;
};
