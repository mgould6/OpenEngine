#include "InputManager.h"
#include <fstream>
#include <sstream>

Camera* InputManager::camera = nullptr;
float InputManager::lastX = 400.0f;
float InputManager::lastY = 300.0f;
bool InputManager::firstMouse = true;
std::map<int, std::function<void()>> InputManager::keyCallbacks;
std::map<int, std::function<void()>> InputManager::mouseButtonCallbacks;
std::map<std::vector<int>, std::function<void()>> InputManager::keyCombinationCallbacks;
std::map<int, bool> InputManager::keyState;
std::map<int, bool> InputManager::mouseButtonState;

void InputManager::processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Movement controls
    if (camera) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera->ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera->ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera->ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera->ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            camera->ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            camera->ProcessKeyboard(DOWN, deltaTime);
    }

    // Custom key callback logic
    for (const auto& callback : keyCallbacks) {
        if (glfwGetKey(window, callback.first) == GLFW_PRESS) {
            keyState[callback.first] = true;
            callback.second();
        }
        else {
            keyState[callback.first] = false;
        }
    }

    // Key combination callback logic
    for (const auto& combo : keyCombinationCallbacks) {
        if (isKeyCombinationPressed(combo.first)) {
            combo.second();
        }
    }
}

void InputManager::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;
   
    if (camera) 
    {
        camera->ProcessMouseMovement(xoffset, yoffset);
    }

    for (const auto& buttonCallback : mouseButtonCallbacks) {
        if (mouseButtonState[buttonCallback.first]) {
            buttonCallback.second();
        }
    }
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        mouseButtonState[button] = true;
        if (mouseButtonCallbacks.find(button) != mouseButtonCallbacks.end()) {
            mouseButtonCallbacks[button]();
        }
    }
    else if (action == GLFW_RELEASE) {
        mouseButtonState[button] = false;
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera->ProcessMouseScroll(yoffset);
}

void InputManager::registerKeyCallback(int key, std::function<void()> callback) {
    keyCallbacks[key] = callback;
}

void InputManager::registerMouseButtonCallback(int button, std::function<void()> callback) {
    mouseButtonCallbacks[button] = callback;
}

void InputManager::registerKeyCombinationCallback(const std::vector<int>& keys, std::function<void()> callback) {
    keyCombinationCallbacks[keys] = callback;
}

void InputManager::loadInputMappings(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key;
        std::vector<int> keys;
        while (ss >> key) {
            keys.push_back(std::stoi(key));
        }
    }

    file.close();
}

void InputManager::saveInputMappings(const std::string& configFile) {
    std::ofstream file(configFile);
    if (!file.is_open()) return;

    for (const auto& mapping : keyCombinationCallbacks) {
        for (int key : mapping.first) {
            file << key << " ";
        }
        file << "\n";
    }

    file.close();
}

bool InputManager::isKeyCombinationPressed(const std::vector<int>& keys) {
    for (int key : keys) {
        if (!keyState[key]) {
            return false;
        }
    }
    return true;
}
