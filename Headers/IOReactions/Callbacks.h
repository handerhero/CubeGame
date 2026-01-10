//
// Created by aspadien on 12.09.2025.
//

#ifndef UNTITLED2_CALLBACKS_H
#define UNTITLED2_CALLBACKS_H

#include <GLFW/glfw3.h>
#include "../Camera/Camera.hpp"
#include <cstdio>

inline void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static bool firstMouse = true;
    static double lastX = 0.0, lastY = 0.0;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // инверсия: вверх = положительное смещение

    lastX = xpos;
    lastY = ypos;
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    int width, height;
    glfwGetWindowSize(window,&width, &height);

    // здесь используете xoffset, yoffset для камеры
    if (!std::isnan(xoffset) && !std::isnan(yoffset)) {

        if (xoffset > 1 || xoffset < -1) {
            camera->camera->rotate((float)(xoffset/width)*90, 0.0f);
        }
        if (yoffset > 1 || yoffset < -1) {
            camera->camera->rotate(0.0f, (float)(yoffset/height)*90);
        }


    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

#endif //UNTITLED2_CALLBACKS_H