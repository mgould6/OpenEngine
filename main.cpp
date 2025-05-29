#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "common_utils/Logger.h"
#include "input/InputManager.h"
#include "model/Camera.h"
#include "shaders/ShaderManager.h"
#include "setup/Setup.h"
#include "render_utils/Renderer.h"
#include "cleanup/Cleanup.h"
#include "setup/Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstddef> // For std::size
#include "setup/GraphicsSetup.h"
#include "setup/InputCallbacks.h"
#include "scene/SceneTest3.h"

// Cube and Plane VAOs and VBOs
unsigned int cubeVAO, cubeVBO;
unsigned int planeVAO, planeVBO;

// Physics objects
btRigidBody* cube;
btRigidBody* ground;

// Callback for window close event
void windowCloseCallback(GLFWwindow* window) {
    Logger::log("Window close event triggered.", Logger::INFO);
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        Logger::log("Failed to initialize GLFW.", Logger::ERROR);
        return -1;
    }

    // Set GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // Create the window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Game Engine", nullptr, nullptr);
    if (!window) {
        Logger::log("Failed to create GLFW window.", Logger::ERROR);
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    glfwSetCursorPosCallback(window, InputManager::mouseCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::log("Failed to initialize GLAD.", Logger::ERROR);
        return -1;
    }

    // Initialize ImGui (only once)
    Renderer::InitializeImGui(window);

    // Initialize Shaders
    if (!ShaderManager::initShaders()) {
        Logger::log("Shader initialization failed.", Logger::ERROR);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Initialize Physics
    PhysicsManager physicsManager;
    physicsManager.Initialize();

    // Run the scene
    SceneTest3(window);

    // Shutdown ImGui
    Renderer::ShutdownImGui();

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
