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
#include "../animation/AnimationController.h" // New: For animation handling
#include "../animation/DebugTools.h" // New: Optional debugging visuals for bones

// Global variables
Camera camera;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
Model* myModel = nullptr;
AnimationController* animationController = nullptr; // New: Animation system controller

void SceneTest2(GLFWwindow* window) {
    Logger::log("Entering SceneTest2.", Logger::INFO);

    // Load the model if not already loaded
    if (!myModel) {
        myModel = new Model("Character Template F3.fbx");
        if (!myModel) {
            Logger::log("Failed to load model.", Logger::ERROR);
            return;
        }
        Logger::log("Model loaded successfully.", Logger::INFO);
    }

    // Initialize the animation controller if not already initialized
    if (!animationController) {
        animationController = new AnimationController(myModel);
        if (!animationController->loadAnimation("Idle", "animations/idle.fbx")) { // Replace with actual animation file
            Logger::log("Failed to load idle animation.", Logger::ERROR);
            return;
        }
        Logger::log("Idle animation loaded successfully.", Logger::INFO);
    }

    // Position the camera to frame the model
    camera.setCameraToFitModel(*myModel);
    Logger::log("Camera positioned to frame the model.", Logger::INFO);

    InputManager::setCamera(&camera);
    camera.SetPerspective(glm::radians(60.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        // Time calculations for frame
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process user input
        InputManager::processInput(window, deltaTime);

        // Begin rendering frame
        Renderer::BeginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update animation
        animationController->update(deltaTime);
        animationController->applyToModel(myModel);

        // Render model with shaders
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

        // Debug visualization for bones (optional)
        DebugTools::renderBoneHierarchy(myModel, camera);

        // Render UI
        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
