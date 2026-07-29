#pragma once

#include <string>

// High-level scene ownership and editor abstraction. Geometry and world-space
// placement live in the concrete object implementations.
class SceneObject {
public:
    virtual ~SceneObject() = default;
    virtual std::string GetName() const = 0;
};
