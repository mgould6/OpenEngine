#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <GLFW/glfw3.h>
#include <map>
#include <string>
#include <functional>
#include "Camera.h"

class InputManager {
public:
    static void setCamera(Camera* cam) { camera = cam; }
    static void processInput(GLFWwindow* window, float deltaTime);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    static void registerKeyCallback(int key, std::function<void()> callback);
    static void registerMouseButtonCallback(int button, std::function<void()> callback);

private:
    static Camera* camera;
    static float lastX, lastY;
    static bool firstMouse;
    static std::map<int, std::function<void()>> keyCallbacks;
    static std::map<int, std::function<void()>> mouseButtonCallbacks;
};

#endif
