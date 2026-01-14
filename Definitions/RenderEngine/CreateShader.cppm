module;
#include <string>
#include <stdexcept>

#include "glad/glad.h"
export module CreateShader;


export void CheckShaderCompileStatus(GLuint shader, const char* name);

export void CheckProgramLinkStatus(GLuint program);

export std::string LoadShaderFromFile(const char* filepath);

export GLuint ShaderCreate(const char* vertexShaderSource, const char* fragmentShaderSource );

export GLuint ShaderCreate(const char* computeShaderSource);




export void* loadTextureRGBA(const std::string& path);