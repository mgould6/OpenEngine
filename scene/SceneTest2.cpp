#include "SceneTest2.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../common_utils/Logger.h"
#include "../model/Camera.h"
#include "../shaders/ShaderManager.h"
#include "../model/Model.h"
#include "../render_utils/Renderer.h"
#include "../setup/Globals.h"
#include "../input/InputManager.h" // Include the InputManager

// Global variables
Camera camera(glm::vec3(0.0f, 1.0f, 10.0f)); // Start further back to see the entire model
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
        else {
            Logger::log("Model loaded successfully.", Logger::INFO);
        }
    }

    // Set up InputManager with the camera
    InputManager::setCamera(&camera);

    // Set up camera perspective
    camera.SetPerspective(glm::radians(90.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        Logger::log("Processing input.", Logger::INFO);
        InputManager::processInput(window, deltaTime); // Process camera controls

        Logger::log("Beginning frame.", Logger::INFO);
        Renderer::BeginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers for the frame
        Renderer::RenderImGui();

        Shader* activeShader = ShaderManager::lightingShader;
        if (activeShader) {
            activeShader->use();

            glm::mat4 lightSpaceMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f) *
                glm::lookAt(glm::vec3(2.0f, 4.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            activeShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

            activeShader->setFloat("lightIntensity", 1.0f);
            activeShader->setInt("pcfKernelSize", 1);
            activeShader->setFloat("shadowBias", 0.005f);
            activeShader->setMat4("view", camera.GetViewMatrix());
            activeShader->setMat4("projection", camera.ProjectionMatrix);
            activeShader->setMat4("model", glm::mat4(1.0f));
            activeShader->setVec3("lightPos", glm::vec3(2.0f, 4.0f, 2.0f));
            activeShader->setVec3("viewPos", camera.Position);

            Logger::log("Drawing model.", Logger::INFO);
            myModel->Draw(*activeShader);
            Logger::log("Model draw call completed.", Logger::INFO);
        }
        else {
            Logger::log("Shader not initialized.", Logger::ERROR);
        }

        Renderer::EndFrame(window);
        Logger::log("Ending frame.", Logger::INFO);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
