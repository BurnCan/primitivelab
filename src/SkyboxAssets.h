#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct SkyboxFaces {
    std::filesystem::path right;
    std::filesystem::path left;
    std::filesystem::path top;
    std::filesystem::path bottom;
    std::filesystem::path front;
    std::filesystem::path back;
};

class SkyboxAssets {
public:
    static std::filesystem::path Root();
    static std::vector<std::string> Discover();
    static std::optional<SkyboxFaces> Resolve(const std::string& name, std::string& error);
};
