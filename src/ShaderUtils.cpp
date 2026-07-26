#include "ShaderUtils.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

std::string LoadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ShaderUtils] ❌ Failed to open: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint CompileShader(GLenum type, const std::string& source, const std::string& label) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_FAILED (" << label << ")\n"
                  << infoLog << std::endl;
    }
    return shader;
}


GLuint CompileShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSource = LoadShaderSource(vertPath);
    std::string fragSource = LoadShaderSource(fragPath);

    if (vertSource.empty() || fragSource.empty()) {
        std::cerr << "[ShaderUtils] ⚠️ Skipping program — missing shader file." << std::endl;
        return 0;
    }

    GLuint vertShader = CompileShader(GL_VERTEX_SHADER, vertSource, vertPath);
    GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, fragSource, fragPath);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[ShaderUtils] ❌ Linking error (" << vertPath << " + " << fragPath << "):\n"
                  << infoLog << std::endl;
    } else {
        std::cout << "[ShaderUtils] ✅ Linked shader program successfully." << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}


