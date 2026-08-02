#include "Skybox.h"
#include "SkyboxAssets.h"
#include <stb_image.h>
#include <array>
#include <iostream>
#include <utility>

Skybox::Skybox(const SkyboxConfig& initial) {
    constexpr float vertices[] = {
        -1,-1,-1,  1,-1,-1,  1, 1,-1,  1, 1,-1, -1, 1,-1, -1,-1,-1,
        -1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
        -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1,-1, -1,-1, 1, -1, 1, 1,
         1, 1, 1,  1, 1,-1,  1,-1,-1,  1,-1,-1,  1,-1, 1,  1, 1, 1,
        -1,-1,-1,  1,-1,-1,  1,-1, 1,  1,-1, 1, -1,-1, 1, -1,-1,-1,
        -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1
    };
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);
    glBindVertexArray(0);
    if (!initial.name.empty()) Reload(initial);
}

Skybox::~Skybox() { Release(); }
Skybox::Skybox(Skybox&& other) noexcept { *this = std::move(other); }
Skybox& Skybox::operator=(Skybox&& other) noexcept {
    if (this != &other) { Release(); vao=std::exchange(other.vao,0); vbo=std::exchange(other.vbo,0);
        cubemap=std::exchange(other.cubemap,0); config=std::move(other.config); lastError=std::move(other.lastError); }
    return *this;
}
void Skybox::Release() {
    if (cubemap) glDeleteTextures(1,&cubemap); if (vbo) glDeleteBuffers(1,&vbo); if (vao) glDeleteVertexArrays(1,&vao);
    cubemap=vbo=vao=0;
}

bool Skybox::Reload(const SkyboxConfig& requested) {
    lastError.clear();
    const auto resolved = SkyboxAssets::Resolve(requested.name, lastError);
    if (!resolved) {
        std::cerr << "[Skybox] " << lastError << (cubemap ? "; previous cubemap retained" : "; skybox remains invalid") << '\n';
        return false;
    }
    const std::array<std::pair<const char*,const std::filesystem::path*>,6> faces{{
        {"right (+X)",&resolved->right},{"left (-X)",&resolved->left},{"top (+Y)",&resolved->top},
        {"bottom (-Y)",&resolved->bottom},{"front (+Z)",&resolved->front},{"back (-Z)",&resolved->back}}};
    GLuint candidate=0; glGenTextures(1,&candidate); glBindTexture(GL_TEXTURE_CUBE_MAP,candidate);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    int expectedW=0, expectedH=0, expectedChannels=0;
    stbi_set_flip_vertically_on_load(false);
    for (std::size_t i=0;i<faces.size();++i) {
        const auto& path=*faces[i].second; int w=0,h=0,channels=0;
        unsigned char* data=stbi_load(path.string().c_str(),&w,&h,&channels,0);
        if (!data) { const char* reason=stbi_failure_reason();lastError="Failed to load " + std::string(faces[i].first) + " face '" + path.string() + "': " + (reason?reason:"unknown image error"); }
        else if (channels!=3 && channels!=4) { lastError="Unsupported channel count for " + std::string(faces[i].first) + " face '" + path.string() + "'"; }
        else if (i && (w!=expectedW || h!=expectedH || channels!=expectedChannels)) { lastError="Dimension/channel mismatch for " + std::string(faces[i].first) + " face '" + path.string() + "'"; }
        if (!data || !lastError.empty()) { if(data)stbi_image_free(data); stbi_set_flip_vertically_on_load(true); glPixelStorei(GL_UNPACK_ALIGNMENT,4); glDeleteTextures(1,&candidate);
            std::cerr<<"[Skybox] "<<lastError<<(cubemap?"; previous cubemap retained":"; skybox remains invalid")<<'\n'; return false; }
        if (!i) { expectedW=w; expectedH=h; expectedChannels=channels; }
        GLenum format=channels==4?GL_RGBA:GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+static_cast<GLenum>(i),0,format,w,h,0,format,GL_UNSIGNED_BYTE,data);
        stbi_image_free(data);
    }
    stbi_set_flip_vertically_on_load(true);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    if(cubemap)glDeleteTextures(1,&cubemap); cubemap=candidate; config=requested; lastError.clear(); return true;
}
void Skybox::Draw() const { if(!IsValid())return; glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,36); glBindVertexArray(0); }
