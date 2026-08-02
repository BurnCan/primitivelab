#pragma once

#include <glad/glad.h>
#include <string>

struct SkyboxConfig {
    std::string name;
};

class Skybox {
public:
    explicit Skybox(const SkyboxConfig& config = {});
    ~Skybox();
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox(Skybox&&) noexcept;
    Skybox& operator=(Skybox&&) noexcept;

    bool Reload(const SkyboxConfig& config);
    void Draw() const;
    bool IsValid() const { return cubemap != 0; }
    GLuint GetCubemapID() const { return cubemap; }
    const SkyboxConfig& GetConfig() const { return config; }
    const std::string& GetLastError() const { return lastError; }

private:
    void Release();
    GLuint vao = 0, vbo = 0, cubemap = 0;
    SkyboxConfig config;
    std::string lastError;
};
