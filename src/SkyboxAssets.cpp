#include "SkyboxAssets.h"
#include "AssetPaths.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

namespace {
constexpr std::array<const char*, 6> FaceNames{{"right", "left", "top", "bottom", "front", "back"}};
constexpr std::array<const char*, 3> Extensions{{".png", ".jpg", ".jpeg"}};

std::string Join(const std::vector<std::string>& values) {
    std::ostringstream result;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) result << ", ";
        result << values[i];
    }
    return result.str();
}
}

std::filesystem::path SkyboxAssets::Root() {
    return ResolveAssetPath("skyboxes", "textures");
}

std::optional<SkyboxFaces> SkyboxAssets::Resolve(const std::string& name, std::string& error) {
    namespace fs = std::filesystem;
    error.clear();
    const fs::path namePath(name);
    if (name.empty() || namePath != namePath.filename() || name == "." || name == "..") {
        error = "Invalid skybox name: '" + name + "'";
        return std::nullopt;
    }

    const fs::path directory = Root() / name;
    std::error_code ec;
    if (!fs::is_directory(directory, ec)) {
        error = "Skybox directory does not exist: " + directory.string();
        if (ec) error += " (" + ec.message() + ")";
        return std::nullopt;
    }

    SkyboxFaces result;
    std::array<fs::path*, 6> destinations{{&result.right, &result.left, &result.top,
                                          &result.bottom, &result.front, &result.back}};
    std::vector<std::string> missing;
    for (std::size_t face = 0; face < FaceNames.size(); ++face) {
        std::vector<fs::path> matches;
        for (const char* extension : Extensions) {
            fs::path candidate = directory / (std::string(FaceNames[face]) + extension);
            ec.clear();
            if (fs::is_regular_file(candidate, ec)) matches.push_back(candidate);
        }
        if (matches.empty()) {
            missing.emplace_back(FaceNames[face]);
        } else if (matches.size() > 1) {
            std::vector<std::string> files;
            for (const auto& match : matches) files.push_back(match.filename().string());
            error = "Skybox '" + name + "' has duplicate files for face '" + FaceNames[face] +
                    "': " + Join(files);
            return std::nullopt;
        } else {
            *destinations[face] = matches.front();
        }
    }
    if (!missing.empty()) {
        error = "Skybox '" + name + "' is missing required faces: " + Join(missing);
        return std::nullopt;
    }
    return result;
}

std::vector<std::string> SkyboxAssets::Discover() {
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    const fs::path root = Root();
    std::error_code ec;
    fs::directory_iterator iterator(root, ec), end;
    if (ec) {
        std::cerr << "[SkyboxAssets] Cannot scan skybox directory '" << root.string()
                  << "': " << ec.message() << '\n';
        return names;
    }
    while (iterator != end) {
        const fs::directory_entry entry = *iterator;
        std::error_code typeError;
        if (entry.is_directory(typeError)) {
            const std::string name = entry.path().filename().string();
            std::string error;
            if (Resolve(name, error)) names.push_back(name);
            else std::cerr << "[SkyboxAssets] Ignoring '" << name << "': " << error << '\n';
        } else if (typeError) {
            std::cerr << "[SkyboxAssets] Cannot inspect '" << entry.path().string()
                      << "': " << typeError.message() << '\n';
        }
        iterator.increment(ec);
        if (ec) {
            std::cerr << "[SkyboxAssets] Error scanning '" << root.string() << "': " << ec.message() << '\n';
            break;
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}
