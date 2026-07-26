#pragma once

#include <filesystem>
#include <string>

// Resolve bundled assets independently of the process working directory.  The
// source-tree fallback is supplied by CMake, while the relative candidates keep
// copied/deployed build layouts working.
inline std::filesystem::path ResolveAssetPath(const std::string& path,
                                              const std::string& assetDirectory) {
    namespace fs = std::filesystem;

    const fs::path requested(path);
    if (requested.is_absolute() && fs::exists(requested)) {
        return requested;
    }

    const fs::path cwd = fs::current_path();
    const fs::path candidates[] = {
        requested,
        cwd / requested,
        cwd.parent_path() / requested,
        cwd / assetDirectory / requested.filename(),
        cwd.parent_path() / assetDirectory / requested.filename(),
        fs::path(PRIMITIVELAB_ASSET_ROOT) / assetDirectory / requested.filename()
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::weakly_canonical(candidate);
        }
    }

    // Return the canonical expected location to make error messages useful.
    return fs::path(PRIMITIVELAB_ASSET_ROOT) / assetDirectory / requested.filename();
}
