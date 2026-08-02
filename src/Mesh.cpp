#include "Mesh.h"

#include <cstddef>
#include <limits>
#include <utility>

Mesh::~Mesh() { Destroy(); }

Mesh::Mesh(Mesh&& other) noexcept
    : vao(std::exchange(other.vao, 0)), vbo(std::exchange(other.vbo, 0)),
      ebo(std::exchange(other.ebo, 0)), indexCount(std::exchange(other.indexCount, 0)) {}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        Destroy();
        vao = std::exchange(other.vao, 0);
        vbo = std::exchange(other.vbo, 0);
        ebo = std::exchange(other.ebo, 0);
        indexCount = std::exchange(other.indexCount, 0);
    }
    return *this;
}

void Mesh::Destroy() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
    indexCount = 0;
}

void Mesh::Upload(const MeshData& data) {
    Destroy();
    indexCount = static_cast<GLsizei>(data.indices.size());
    if (data.vertices.empty() || data.indices.empty()) return;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), data.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), data.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void Mesh::Draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

namespace {
glm::vec3 ClosestPoint(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 ab=b-a, ac=c-a, ap=p-a; const float d1=glm::dot(ab,ap), d2=glm::dot(ac,ap);
    if(d1<=0&&d2<=0)return a; const glm::vec3 bp=p-b; const float d3=glm::dot(ab,bp),d4=glm::dot(ac,bp);
    if(d3>=0&&d4<=d3)return b; const float vc=d1*d4-d3*d2;
    if(vc<=0&&d1>=0&&d3<=0)return a+ab*(d1/(d1-d3)); const glm::vec3 cp=p-c;
    const float d5=glm::dot(ab,cp),d6=glm::dot(ac,cp); if(d6>=0&&d5<=d6)return c;
    const float vb=d5*d2-d1*d6; if(vb<=0&&d2>=0&&d6<=0)return a+ac*(d2/(d2-d6));
    const float va=d3*d6-d5*d4; if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0)return b+(c-b)*((d4-d3)/((d4-d3)+(d5-d6)));
    const float inv=1.0f/(va+vb+vc); return a+ab*(vb*inv)+ac*(vc*inv);
}
}

bool IntersectsSphereMesh(const glm::vec3& center, float radius, const MeshData& geometry,
                          const glm::mat4& model, glm::vec3* contactNormal) {
    const float r2=radius*radius; float best=std::numeric_limits<float>::max(); bool hit=false; glm::vec3 normal(0,1,0);
    for(std::size_t i=0;i+2<geometry.indices.size();i+=3) {
        const auto ia=geometry.indices[i], ib=geometry.indices[i+1], ic=geometry.indices[i+2];
        if(ia>=geometry.vertices.size()||ib>=geometry.vertices.size()||ic>=geometry.vertices.size()) continue;
        const glm::vec3 a=model*glm::vec4(geometry.vertices[ia].position,1), b=model*glm::vec4(geometry.vertices[ib].position,1), c=model*glm::vec4(geometry.vertices[ic].position,1);
        const glm::vec3 delta=center-ClosestPoint(center,a,b,c); const float d2=glm::dot(delta,delta);
        if(d2<=r2&&d2<best){hit=true;best=d2;if(d2>1e-7f)normal=delta/glm::sqrt(d2);else{auto n=glm::cross(b-a,c-a);if(glm::dot(n,n)>1e-7f)normal=glm::normalize(n);}}
    }
    if(hit&&contactNormal)*contactNormal=normal; return hit;
}
