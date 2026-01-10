//
// Created by aspadien on 11.11.2025.
//

#ifndef CUBEREBUILD_GLDEBUG_HPP
#define CUBEREBUILD_GLDEBUG_HPP
#include "glad/glad.h"

void GLAPIENTRY MessageCallback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,const void* userParam);

#endif //CUBEREBUILD_GLDEBUG_HPP