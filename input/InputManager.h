#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <GLFW/glfw3.h>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include "../model/Camera.h"

class InputManager {
public:
    static void setCamera(Camera* cam) { camera = cam; }
    static void processInput(GLFWwindow* window, float deltaTime);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    static void registerKeyCallback(int key, std::function<void()> callback);
    static void registerMouseButtonCallback(int button, std::function<void()> callback);
    static void registerKeyCombinationCallback(const std::vector<int>& keys, std::function<void()> callback);

    static void loadInputMappings(const std::string& configFile);
    static void saveInputMappings(const std::string& configFile);

private:
    static Camera* camera;
    static float lastX, lastY;
    static bool firstMouse;
    static std::map<int, std::function<void()>> keyCallbacks;
    static std::map<int, std::function<void()>> mouseButtonCallbacks;
    static std::map<std::vector<int>, std::function<void()>> keyCombinationCallbacks;
    static std::map<int, bool> keyState;
    static std::map<int, bool> mouseButtonState;

    static bool isKeyCombinationPressed(const std::vector<int>& keys);
};

#endif
