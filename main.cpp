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

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Model* myModel = nullptr;

unsigned int depthMapFBO[Renderer::NUM_CASCADES], hdrFBO, depthMap[Renderer::NUM_CASCADES];
unsigned int colorBuffers[2], pingpongFBO[2], pingpongBuffer[2];
unsigned int VAO, VBO;

int main() {
    GLFWwindow* window;

    if (!initialize(window, SCR_WIDTH, SCR_HEIGHT)) {
        return -1;
    }

    if (!ShaderManager::initShaders()) {
        Logger::log("Failed to initialize shaders", Logger::ERROR);
        return -1;
    }

    if (!Renderer::initShadowMapping()) {
        Logger::log("Failed to initialize shadow mapping", Logger::ERROR);
        return -1;
    }

    if (!Renderer::initSSAO()) {
        Logger::log("Failed to initialize SSAO", Logger::ERROR);
        return -1;
    }

    Shader brightExtractShader("shaders/post_processing/bright_extract.vs", "shaders/post_processing/bright_extract.fs");
    Shader blurShader("shaders/post_processing/blur.vs", "shaders/post_processing/blur.fs");
    Shader combineShader("shaders/post_processing/combine.vs", "shaders/post_processing/combine.fs");

    if (!brightExtractShader.isCompiled() || !blurShader.isCompiled() || !combineShader.isCompiled()) {
        Logger::log("Shader compilation failed", Logger::ERROR);
        return -1;
    }

    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::log("Framebuffer not complete!", Logger::ERROR);
        return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::log("Pingpong framebuffer not complete!", Logger::ERROR);
            return -1;
        }
    }

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

    myModel = new Model("C:/OpenGL/models/FinalBaseMesh.obj");
    if (!myModel) {
        Logger::log("Failed to load model", Logger::ERROR);
        return -1;
    }
    camera.setCameraToFitModel(*myModel);
    InputManager::setCamera(&camera);

    InputManager::registerKeyCallback(GLFW_KEY_W, []() { camera.ProcessKeyboard(FORWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_S, []() { camera.ProcessKeyboard(BACKWARD, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_A, []() { camera.ProcessKeyboard(LEFT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_D, []() { camera.ProcessKeyboard(RIGHT, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_E, []() { camera.ProcessKeyboard(UP, deltaTime); });
    InputManager::registerKeyCallback(GLFW_KEY_C, []() { camera.ProcessKeyboard(DOWN, deltaTime); });

    Renderer::InitializeImGui(window);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);

        camera.MovementSpeed = Renderer::getCameraSpeed();

        Renderer::render(window, deltaTime);
    }

    Renderer::ShutdownImGui();

    cleanup();
    return 0;
}
