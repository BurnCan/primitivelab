#pragma once
#include "ShaderUtils.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>
class MaterialRegistry {
public:
 explicit MaterialRegistry(const std::filesystem::path& shaderRoot);
 void Clear(){shaders.clear();}
 const std::vector<std::string>& GetIds() const{return ids;}
 const Shader* Find(const std::string&) const;
 const Shader* Resolve(const std::string&) const;
 std::string GetWarning(const std::string&) const;
private:
 std::vector<std::string> ids;
 std::map<std::string,Shader> shaders;
 std::map<std::string,std::string> errors;
};
