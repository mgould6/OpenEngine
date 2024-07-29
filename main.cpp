#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "common_utils/Logger.h"
#include "input/InputManager.h"
#include "engine/Engine.h"
#include "model/Camera.h"
#include "model/Model.h"
#include "shaders/ShaderManager.h"
#include "setup/Setup.h"
#include "render_utils/Renderer.h"
#include "cleanup/Cleanup.h"
#include "Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Function Prototypes
bool initializeGraphics(GLFWwindow*& window);
void setupFramebuffers();
void setupVertexData();
void registerInputCallbacks();
void cleanupResources();

int main() {
    GLFWwindow* window;

    if (!initializeGraphics(window)) {
        return -1;
    }

    setupFramebuffers();
    setupVertexData();
    registerInputCallbacks();

    myModel = new Model("C:/OpenGL/models/FinalBaseMesh.obj");
    if (!myModel) {
        Logger::log("Failed to load model", Logger::ERROR);
        cleanupResources();
        return -1;
    }

    camera.setCameraToFitModel(*myModel);
    InputManager::setCamera(&camera);

    Renderer::InitializeImGui(window);
    Renderer::physicsManager.Initialize(); // Initialize PhysicsManager

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        camera.MovementSpeed = Renderer::getCameraSpeed();
        Renderer::render(window, deltaTime);
    }

    Renderer::ShutdownImGui();
    cleanupResources();
    return 0;
}

bool initializeGraphics(GLFWwindow*& window) {
    if (!initialize(window, SCR_WIDTH, SCR_HEIGHT)) {
        return false;
    }

    if (!ShaderManager::initShaders()) {
        Logger::log("Failed to initialize shaders", Logger::ERROR);
        return false;
    }

    if (!Renderer::initShadowMapping()) {
        Logger::log("Failed to initialize shadow mapping", Logger::ERROR);
        return false;
    }

    if (!Renderer::initSSAO()) {
        Logger::log("Failed to initialize SSAO", Logger::ERROR);
        return false;
    }

    return true;
}

void setupFramebuffers() {
    Renderer::createFramebuffer(hdrFBO, colorBuffers[0], SCR_WIDTH, SCR_HEIGHT, GL_RGBA16F);
    Renderer::createPingPongFramebuffers(pingpongFBO, pingpongBuffer, SCR_WIDTH, SCR_HEIGHT);
}

void setupVertexData() {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.5f, 0.0f,  0.0f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void registerInputCallbacks() {
    InputManager::registerKeyCallback(GLFW_KEY_W, []() { camera.ProcessKeyboard(FORWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_S, []() { camera.ProcessKeyboard(BACKWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_A, []() { camera.ProcessKeyboard(LEFT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_D, []() { camera.ProcessKeyboard(RIGHT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_E, []() { camera.ProcessKeyboard(UP, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_C, []() { camera.ProcessKeyboard(DOWN, deltaTime); });
}

void cleanupResources() {
    delete myModel;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(2, colorBuffers);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongBuffer);
    glfwTerminate();
}
