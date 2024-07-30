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
#include "Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Function Prototypes
bool initializeGraphics(GLFWwindow*& window);
void setupFramebuffers();
void setupCubeVertexData();
void setupPlaneVertexData();
void registerInputCallbacks();
void cleanupResources();
void renderCube(const Shader& shader);
void renderPlane(const Shader& shader);

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

    setupFramebuffers();
    setupCubeVertexData();
    setupPlaneVertexData();
    registerInputCallbacks();

    // Adjust the camera to view the scene with the cube and plane
    camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f)); // Positioned to view the cube dropping onto the plane
    InputManager::setCamera(&camera);

    Renderer::InitializeImGui(window);
    Renderer::physicsManager.Initialize(); // Initialize PhysicsManager

    // Create a plane
    ground = Renderer::physicsManager.CreatePlane(btVector3(0, 1, 0), 1);

    // Create a cube
    cube = Renderer::physicsManager.CreateCube(1.0f, 1.0f, btVector3(0, 10, 0));

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

void setupCubeVertexData() {
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void setupPlaneVertexData() {
    float planeVertices[] = {
        // positions            // normals
        5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,
       -5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,
       -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,

        5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,
       -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,
        5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
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
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(2, colorBuffers);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongBuffer);
    glfwTerminate();
}
