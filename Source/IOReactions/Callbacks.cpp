module;
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
import Camera;
module Callbacks;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
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
            camera->camera->rotate(static_cast<float>(xoffset / width)*90, 0.0f);
        }
        if (yoffset > 1 || yoffset < -1) {
            camera->camera->rotate(0.0f, static_cast<float>(yoffset / height)*90);
        }


    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}