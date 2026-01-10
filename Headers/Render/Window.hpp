//
// Created by aspadien on 07.01.2026.
//

#ifndef CUBEREBUILD_WINDOW_HPP
#define CUBEREBUILD_WINDOW_HPP
#include "Camera/Camera.hpp"
#include "glad/glad.h"
#include "IOReactions/Callbacks.h"

class Window {
    public:
    GLFWwindow* window;

    explicit Window(Camera& camera , const char * windowName = "Dynamic World Example");

private:
    // Хелперы для инициализации
    void initGLFW();
    void configureWindowHints();
    void createWindowObject(const char* title);
    void initGLAD();
    void configureOpenGLState();

    // Коллбеки лучше делать статическими
    //static void error_callback(int error, const char* description);
    //static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
#endif //CUBEREBUILD_WINDOW_HPP