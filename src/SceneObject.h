#pragma once

#include <optional>
#include <string>
#include <variant>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Primitives.h"
#include "Terrain.h"

struct Transform {
    glm::vec3 position{0.0f}, rotation{0.0f}, scale{1.0f};
    glm::mat4 Matrix() const {
        glm::mat4 result = glm::translate(glm::mat4(1.0f), position);
        result = glm::rotate(result, glm::radians(rotation.z), {0,0,1});
        result = glm::rotate(result, glm::radians(rotation.y), {0,1,0});
        result = glm::rotate(result, glm::radians(rotation.x), {1,0,0});
        return glm::scale(result, scale);
    }
    glm::vec3 Forward() const {
        return glm::normalize(glm::vec3(Matrix() * glm::vec4(0, 0, -1, 0)));
    }
};
enum class LightType { Point, Directional, Spot };
struct LightComponent {
    LightType type = LightType::Point;
    glm::vec3 color{1.0f};
    float intensity = 1.0f, range = 10.0f;
    float innerConeAngle = 20.0f, outerConeAngle = 30.0f;
};
using SceneObjectPayload = std::variant<std::monostate, Primitive2D, Primitive3D, Terrain>;

class SceneObject {
public:
    template<class Payload, class... Args>
    SceneObject(std::string objectName, std::in_place_type_t<Payload>, Args&&... args)
        : name(std::move(objectName)), payload(std::in_place_type<Payload>, std::forward<Args>(args)...) {}
    std::string name;
    Transform transform;
    bool enabled = true, visible = true;
    SceneObjectPayload payload;
    std::optional<LightComponent> light;
};
