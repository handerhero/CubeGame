//
// Created by aspadien on 12.09.2025.
//

#ifndef UNTITLED2_KEYBOARDCONTROL_H
#define UNTITLED2_KEYBOARDCONTROL_H
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

#include "../Camera/Camera.hpp"
class KeyboardControl {
    public:
    glm::vec3 moveVector = glm::vec3(0.0f);
    int axisX = 0;
    int axisZ = 0;
    KeyboardControl() = default;

    void keyboardControlNotFree(Camera* camera, GLFWwindow* window, double time, glm::vec3 *direction, bool onGround, float jumpPower, float speed) {
            const auto frameTime = time;
            moveVector = glm::vec3(0.0f);
            axisX = 0;
            axisZ = 0;
        if (!onGround) {
            speed /= 45;
        }


            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ){
                moveVector += glm::vec3(camera->camera->forward().x, 0.0f,camera->camera->forward().z) * speed * frameTime;
                axisX++;
            }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ){
                    moveVector -= glm::vec3(camera->camera->forward().x, 0.0f,camera->camera->forward().z) * speed * frameTime;
                    axisX++;
                }
                    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ) {
                        moveVector += camera->camera->right() * speed  * frameTime;
                        axisZ++;
                    }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ) {
                    moveVector -= camera->camera->right() * speed  * frameTime;
                    axisZ++;
                }
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && onGround ) {
                    *direction += glm::vec3(0.0f,jumpPower,0.0f);
                }
                if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ) {
                    std::cout << camera->pos << std::endl;
                }
        float multiplier = sqrt((axisX%2)+(axisZ%2));
        if (sqrt(multiplier)!=0){
        *direction +=moveVector / multiplier;
            }
        }

    void keyboardControlFreeCam(Camera* camera, GLFWwindow* window) {
        constexpr float speed = 4.0f;


        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ){
            camera->pos += glm::vec3(camera->camera->forward().x, 0.0f,camera->camera->forward().z) * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ){
            camera->pos -= glm::vec3(camera->camera->forward().x, 0.0f,camera->camera->forward().z) * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ) {
            camera->pos += camera->camera->right() * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ) {
            camera->pos -= camera->camera->right() * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera->pos += glm::vec3(0,1,0) * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) {
            camera->pos -= glm::vec3(0,1,0) * speed;
        }
    }
};
#endif //UNTITLED2_KEYBOARDCONTROL_H