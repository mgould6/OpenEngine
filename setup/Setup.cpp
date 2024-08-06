#include "Setup.h"
#include "../common_utils/Logger.h"
#include "../input/InputManager.h"
#include "../model/Camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Globals.h" 


void framebuffer_size_callback(GLFWwindow* window, int width, int height);

bool initialize(GLFWwindow*& window, const unsigned int width, const unsigned int height) {
    if (!glfwInit()) {
        Logger::log("Failed to initialize GLFW", Logger::ERROR);
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "OpenGL Game Engine", NULL, NULL);
    if (window == NULL) {
        Logger::log("Failed to create GLFW window", Logger::ERROR);
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::log("Failed to initialize GLAD", Logger::ERROR);
        return false;
    }

    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, InputManager::mouseCallback); // Add mouse callback
    glfwSetScrollCallback(window, InputManager::scrollCallback); // Add scroll callback
    glfwSetMouseButtonCallback(window, InputManager::mouseButtonCallback); // Add mouse button callback

    glEnable(GL_DEPTH_TEST);

    return true;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    camera.ProjectionMatrix = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
}
