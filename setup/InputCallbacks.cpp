#include "InputCallbacks.h"
#include "input/InputManager.h"
#include "model/Camera.h"
#include "Globals.h"

void registerInputCallbacks() {
    InputManager::registerKeyCallback(GLFW_KEY_W, []() { camera.ProcessKeyboard(FORWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_S, []() { camera.ProcessKeyboard(BACKWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_A, []() { camera.ProcessKeyboard(LEFT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_D, []() { camera.ProcessKeyboard(RIGHT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_E, []() { camera.ProcessKeyboard(UP, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_C, []() { camera.ProcessKeyboard(DOWN, deltaTime); });
}
