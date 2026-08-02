#pragma once

#include <string>
#include <variant>

struct TriangleMeshSource {};
struct PlaneMeshSource {};
struct CubeMeshSource {};
struct SlabMeshSource {};
struct TriangularPrismMeshSource {};

struct SphereMeshSource {
    unsigned int xSegments = 32;
    unsigned int ySegments = 32;
};

enum class TerrainGeometryType { Flat, Heightmap };
enum class TerrainPlane { XY, XZ, YZ };

struct TerrainMeshSource {
    TerrainGeometryType geometryType = TerrainGeometryType::Flat;
    TerrainPlane plane = TerrainPlane::XZ;
    int widthSegments = 20;
    int depthSegments = 20;
    float width = 20.0f;
    float depth = 20.0f;
    float heightScale = 1.0f;
    std::string heightmapPath;
};

// Imported models can be added as another alternative once a real asset loader
// exists. Neither Mesh nor MeshObject needs to change for that extension.
using MeshSource = std::variant<TriangleMeshSource, PlaneMeshSource, CubeMeshSource,
                                SlabMeshSource, TriangularPrismMeshSource,
                                SphereMeshSource, TerrainMeshSource>;

namespace MeshSources {
MeshSource Triangle();
MeshSource Plane();
MeshSource Cube();
MeshSource Slab();
MeshSource TriangularPrism();
MeshSource Sphere(unsigned int xSegments = 32, unsigned int ySegments = 32);
MeshSource Terrain(const TerrainMeshSource& source = {});
}
