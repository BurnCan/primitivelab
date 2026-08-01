#include "ShaderUtils.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>
std::string LoadShaderSource(const std::string& path){std::ifstream f(path);if(!f){std::cerr<<"[Shader] Failed to open "<<path<<'\n';return {};}std::stringstream s;s<<f.rdbuf();return s.str();}
GLuint CompileShader(GLenum type,const std::string& source,const std::string& label){if(source.empty())return 0;GLuint s=glCreateShader(type);const char*p=source.c_str();glShaderSource(s,1,&p,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[4096];glGetShaderInfoLog(s,sizeof log,nullptr,log);std::cerr<<"[Shader] Compile failed ("<<label<<"): "<<log<<'\n';glDeleteShader(s);return 0;}return s;}
GLuint CompileShaderProgram(const std::string& v,const std::string& f){Shader shader(v,f);GLuint id=shader.ID;shader.ID=0;return id;}
Shader::Shader(const std::string& vp,const std::string& fp){GLuint v=CompileShader(GL_VERTEX_SHADER,LoadShaderSource(vp),vp),f=CompileShader(GL_FRAGMENT_SHADER,LoadShaderSource(fp),fp);if(!v||!f){lastError="Shader source missing or compilation failed";if(v)glDeleteShader(v);if(f)glDeleteShader(f);return;}ID=glCreateProgram();glAttachShader(ID,v);glAttachShader(ID,f);glLinkProgram(ID);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(ID,GL_LINK_STATUS,&ok);if(!ok){char log[4096];glGetProgramInfoLog(ID,sizeof log,nullptr,log);lastError=log;std::cerr<<"[Shader] Link failed ("<<vp<<" + "<<fp<<"): "<<lastError<<'\n';glDeleteProgram(ID);ID=0;}}
Shader::~Shader(){if(ID)glDeleteProgram(ID);}
Shader::Shader(Shader&& o)noexcept:ID(std::exchange(o.ID,0)),lastError(std::move(o.lastError)){}
Shader& Shader::operator=(Shader&& o)noexcept{if(this!=&o){if(ID)glDeleteProgram(ID);ID=std::exchange(o.ID,0);lastError=std::move(o.lastError);}return *this;}
void Shader::SetMat4(const std::string&n,const glm::mat4&m)const{glUniformMatrix4fv(glGetUniformLocation(ID,n.c_str()),1,GL_FALSE,glm::value_ptr(m));}
void Shader::SetVec3(const std::string&n,const glm::vec3&v)const{glUniform3fv(glGetUniformLocation(ID,n.c_str()),1,glm::value_ptr(v));}
void Shader::SetFloat(const std::string&n,float v)const{glUniform1f(glGetUniformLocation(ID,n.c_str()),v);}
void Shader::SetInt(const std::string&n,int v)const{glUniform1i(glGetUniformLocation(ID,n.c_str()),v);}
