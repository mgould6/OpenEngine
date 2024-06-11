#include "InputManager.h"

Camera* InputManager::camera = nullptr;
float InputManager::lastX = 400.0f;
float InputManager::lastY = 300.0f;
bool InputManager::firstMouse = true;
std::map<int, std::function<void()>> InputManager::keyCallbacks;
std::map<int, std::function<void()>> InputManager::mouseButtonCallbacks;

void InputManager::processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Process registered key callbacks
    for (const auto& callback : keyCallbacks) {
        if (glfwGetKey(window, callback.first) == GLFW_PRESS)
            callback.second();
    }
}

void InputManager::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera->ProcessMouseMovement(xoffset, yoffset);
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
