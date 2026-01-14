module;
#include <GLFW/glfw3.h>
import Camera;
export module Callbacks;

export void mouse_callback(GLFWwindow* window, double xpos, double ypos);

export void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
export void error_callback(int error, const char* description);