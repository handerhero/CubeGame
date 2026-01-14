module;

#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

import Camera;

export module Keyboard;

export class Keyboard {
    public:
    glm::vec3 moveVector = glm::vec3(0.0f);
    int axisX = 0;
    int axisZ = 0;
    Keyboard() = default;

    void keyboardControlNotFree(Camera* camera, GLFWwindow* window, double time, glm::vec3 *direction, bool onGround, float jumpPower, float speed);

    void keyboardControlFreeCam(Camera* camera, GLFWwindow* window);
};