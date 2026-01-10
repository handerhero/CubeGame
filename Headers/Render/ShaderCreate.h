//
// Created by aspadien on 01.08.25.
//

#ifndef SHADERCREATE_H
#define SHADERCREATE_H

#include <fstream>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>

#include "glad/glad.h"
#include "../Libs/stb_image.h"



inline void CheckShaderCompileStatus(GLuint shader, const char* name) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "ERROR compiling shader [" << name << "]:\n" << log.data() << std::endl;
    }
}

inline void CheckProgramLinkStatus(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "ERROR linking program:\n" << log.data() << std::endl;
    }
}

inline std::string LoadShaderFromFile(const char* filepath) {
    // Открываем в бинарном режиме, чтобы не было авто-преобразования \r\n
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open shader file: ") + filepath);
    }

    // Перемещаемся в конец, чтобы узнать размер
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Выделяем строку нужного размера
    std::string buffer(size, '\0');
    if (!file.read(&buffer[0], size)) {
        throw std::runtime_error(std::string("Failed to read shader file: ") + filepath);
    }

    // Удаляем BOM, если он есть (только для UTF-8)
    if (buffer.size() >= 3 &&
        static_cast<unsigned char>(buffer[0]) == 0xEF &&
        static_cast<unsigned char>(buffer[1]) == 0xBB &&
        static_cast<unsigned char>(buffer[2]) == 0xBF) {
        buffer.erase(0, 3);
        }

    return buffer;
}

inline GLuint ShaderCreate(const char* vertexShaderSource, const char* fragmentShaderSource ) { //const char* geometryShaderSource
    GLuint vertex_shader, fragment_shader, geometry_shader, program;
    std::string vertex_shader_code = LoadShaderFromFile(vertexShaderSource);
    const char* vertex_shader_text = vertex_shader_code.c_str();

    std::string fragment_shader_code = LoadShaderFromFile(fragmentShaderSource);
    const char* fragment_shader_text = fragment_shader_code.c_str();

   // std::string geometry_shader_code = LoadShaderFromFile(geometryShaderSource);
   // const char* geometry_shader_text = geometry_shader_code.c_str();

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    CheckShaderCompileStatus(vertex_shader, "vertex");

   // geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
    //glShaderSource(geometry_shader, 1, &geometry_shader_text, NULL);
   // glCompileShader(geometry_shader);
   // CheckShaderCompileStatus(geometry_shader, "geometry");

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
   // CheckShaderCompileStatus(fragment_shader, "fragment");



    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
   // glAttachShader(program, geometry_shader);
    glAttachShader(program, fragment_shader);


    glLinkProgram(program);
  //  CheckProgramLinkStatus(program);

    return program;
}

inline GLuint ShaderCreate(const char* computeShaderSource) { //const char* geometryShaderSource
    GLuint compute_shader, program;
    std::string compute_shader_code = LoadShaderFromFile(computeShaderSource);
    const char* compute_shader_text = compute_shader_code.c_str();


    compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &compute_shader_text, NULL);
    glCompileShader(compute_shader);


    program = glCreateProgram();
    glAttachShader(program, compute_shader);

    glLinkProgram(program);
    CheckProgramLinkStatus(program);

    return program;
}




inline void* loadTextureRGBA(const std::string& path) {
    int w,h,channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data) throw std::runtime_error("failed to load " + path);
    return data;
}

#endif //SHADERCREATE_H
