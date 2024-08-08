#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "common_utils/Logger.h"
#include "input/InputManager.h"
#include "engine/Engine.h"
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
#include "scene/SceneTest1.h"

// Cube and Plane VAOs and VBOs
unsigned int cubeVAO, cubeVBO;
unsigned int planeVAO, planeVBO;

// Physics objects
btRigidBody* cube;
btRigidBody* ground;

// main.cpp
int main() {
    GLFWwindow* window;

    if (!initializeGraphics(window)) {
        return -1;
    }

    SceneTest1(window);

    // Variables to keep track of FPS
    int frameCount = 0;
    double fpsStartTime = glfwGetTime();
    const double fpsUpdateInterval = 0.01; // Update FPS every .1 seconds

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;

        // Calculate and print FPS
        if (currentFrame - fpsStartTime >= fpsUpdateInterval) {
            double fps = frameCount / fpsUpdateInterval;
            std::cout << "FPS: " << fps << std::endl;
            frameCount = 0;
            fpsStartTime = currentFrame;
        }

        InputManager::processInput(window, deltaTime);
        camera.MovementSpeed = Renderer::getCameraSpeed();
        Renderer::render(window, deltaTime);
    }

    Renderer::ShutdownImGui();
    cleanupResources();
    return 0;
}
