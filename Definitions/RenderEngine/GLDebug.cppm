module;
#include "glad/glad.h"

export module GLDebug;
export void GLAPIENTRY MessageCallback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,const void* userParam);
