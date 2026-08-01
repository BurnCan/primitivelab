#include "MaterialRegistry.h"
#include <algorithm>
#include <cctype>
#include <iostream>
MaterialRegistry::MaterialRegistry(const std::filesystem::path& root){namespace fs=std::filesystem;auto dir=root/"material";std::map<std::string,fs::path> found;std::error_code ec;for(fs::directory_iterator it(dir,ec),end;!ec&&it!=end;it.increment(ec)){if(!it->is_regular_file()||it->path().extension()!=".frag")continue;auto id=it->path().stem().string();std::transform(id.begin(),id.end(),id.begin(),[](unsigned char c){return char(std::tolower(c));});if(!found.emplace(id,it->path()).second){errors[id]="duplicate normalized material ID";std::cerr<<"[Materials] Duplicate ID: "<<id<<'\n';}}
 for(auto&[id,path]:found)ids.push_back(id);std::sort(ids.begin(),ids.end());auto d=std::find(ids.begin(),ids.end(),"default");if(d!=ids.end())std::rotate(ids.begin(),d,d+1);for(auto&id:ids){if(errors.count(id))continue;Shader shader((root/"engine/material.vert").string(),found[id].string());if(!shader.IsValid()){errors[id]=shader.GetLastError();std::cerr<<"[Materials] Invalid material "<<id<<": "<<errors[id]<<'\n';}shaders.emplace(id,std::move(shader));}}
const Shader* MaterialRegistry::Find(const std::string&id)const{auto i=shaders.find(id);return i==shaders.end()||!i->second.IsValid()?nullptr:&i->second;}
const Shader* MaterialRegistry::Resolve(const std::string&id)const{if(auto*s=Find(id))return s;return Find("default");}
std::string MaterialRegistry::GetWarning(const std::string&id)const{if(Find(id))return {};auto e=errors.find(id);return e==errors.end()?"Material '"+id+"' is missing; using fallback.":"Material '"+id+"' failed: "+e->second;}
