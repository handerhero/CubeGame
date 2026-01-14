module;
#include "GLFW/glfw3.h"
export module Window;

import Callbacks;
import Camera;

export class Window {
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
};