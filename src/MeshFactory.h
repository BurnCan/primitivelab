#pragma once

#include "Mesh.h"
#include "MeshSource.h"

class MeshFactory {
public:
    static MeshData Create(const MeshSource& source);
};
