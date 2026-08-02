#pragma once

#include <string>
#include <vector>

// Texture discovery remains a general editor utility. Mesh creation is exposed
// through MeshSource, MeshFactory, and MeshObject.
std::vector<std::string> GetAvailableTextures(const std::string& directory);
