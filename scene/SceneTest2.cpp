#include "SceneTest2.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../common_utils/Logger.h"
#include "../model/Camera.h"
#include "../shaders/ShaderManager.h"
#include "../model/Model.h"
#include "../render_utils/Renderer.h"
#include "../setup/Globals.h"
#include "../input/InputManager.h"

// Global variables
Camera camera;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
Model* myModel = nullptr;

void SceneTest2(GLFWwindow* window) {
    Logger::log("Entering SceneTest2.", Logger::INFO);

    if (!myModel) {
        myModel = new Model("Character Template F3.fbx");
        if (!myModel) {
            Logger::log("Failed to load model.", Logger::ERROR);
            return;
        }
        Logger::log("Model loaded successfully.", Logger::INFO);
    }

    camera.setCameraToFitModel(*myModel);
    Logger::log("Camera positioned to frame the model.", Logger::INFO);

    InputManager::setCamera(&camera);
    camera.SetPerspective(glm::radians(60.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        Renderer::BeginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (Shader* activeShader : ShaderManager::allShaders) {
            if (!activeShader) continue;

            activeShader->use();

            glm::mat4 lightSpaceMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f) *
                                         glm::lookAt(glm::vec3(2.0f, 4.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            if (activeShader->hasUniform("lightSpaceMatrix")) {
                activeShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
            }
            if (activeShader->hasUniform("lightIntensity")) {
                activeShader->setFloat("lightIntensity", Renderer::getLightIntensity());
            }
            activeShader->setMat4("view", camera.GetViewMatrix());
            activeShader->setMat4("projection", camera.ProjectionMatrix);
            activeShader->setMat4("model", glm::mat4(1.0f));

            myModel->Draw(*activeShader);
        }

        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
