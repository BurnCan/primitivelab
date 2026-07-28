#pragma once

#include <string>

enum class SceneDimension { TwoD, ThreeD };

// High-level scene ownership and editor abstraction. Geometry and world-space
// placement live in the concrete object implementations.
class SceneObject {
public:
    virtual ~SceneObject() = default;
    virtual SceneDimension GetDimension() const = 0;
    virtual std::string GetName() const = 0;
};

namespace TwoD {
class SceneObject : public ::SceneObject {
public:
    SceneDimension GetDimension() const final { return SceneDimension::TwoD; }
    // Future TwoD::Model types inherit directly from this class.
};
}

namespace ThreeD {
class SceneObject : public ::SceneObject {
public:
    SceneDimension GetDimension() const final { return SceneDimension::ThreeD; }
    // Future ThreeD::Model types inherit directly from this class.
};
}
