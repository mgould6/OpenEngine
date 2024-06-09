// InputManager.h
#pragma once

#include <GLFW/glfw3.h>
#include "Camera.h"

class InputManager {
public:
    static void processInput(GLFWwindow* window, float deltaTime);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    static void setCamera(Camera* cam);

private:
    static Camera* camera;
    static bool firstMouse;
    static float lastX;
    static float lastY;
};
