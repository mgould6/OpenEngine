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
#include "../animation/AnimationController.h"
#include <filesystem>

// Global variables
Camera camera;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
Model* myModel = nullptr;
AnimationController* animationController = nullptr;

void SceneTest2(GLFWwindow* window) {
    Logger::log("Entering SceneTest2.", Logger::INFO);

    // Load the model if not already loaded
    if (!myModel) {
        myModel = new Model("Character Template F9.fbx");
        if (!myModel) {
            Logger::log("ERROR: Failed to load model.", Logger::ERROR);
            return;
        }
        Logger::log("INFO: Model loaded successfully.", Logger::INFO);
    }

    // Debug: Print model size and center
    glm::vec3 modelCenter = myModel->getBoundingBoxCenter();
    float modelSize = myModel->getBoundingBoxRadius();
    Logger::log("DEBUG: Model Center at (" + std::to_string(modelCenter.x) + ", " +
        std::to_string(modelCenter.y) + ", " + std::to_string(modelCenter.z) + ")", Logger::INFO);
    Logger::log("DEBUG: Model Size (Radius): " + std::to_string(modelSize), Logger::INFO);

    // Position the camera relative to the model using setCameraToFitModel()
    camera.setCameraToFitModel(*myModel);

    InputManager::setCamera(&camera);
    camera.SetPerspective(glm::radians(60.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    glEnable(GL_DEPTH_TEST);

    // Initialize the animation controller
    if (!animationController) {
        animationController = new AnimationController(myModel);
        if (!animationController->loadAnimation("rig.001|Idle", "Character Template F9.fbx")) {
            Logger::log("ERROR: AnimationController failed to load animation!", Logger::ERROR);
            return;
        }
        Logger::log("INFO: Successfully loaded animation rig.001|Idle.", Logger::INFO);

        // Manually set the animation to test if it applies
        animationController->setCurrentAnimation("rig.001|Idle");
        Logger::log("INFO: Set current animation to rig.001|Idle.", Logger::INFO);
    }

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);

        Renderer::BeginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (animationController) {
            Logger::log("DEBUG: Calling animation update", Logger::INFO);
            animationController->update(deltaTime);
            Logger::log("DEBUG: Calling applyToModel", Logger::INFO);
            animationController->applyToModel(myModel);
        }

        // Ensure the correct shader is used
        Shader* activeShader = ShaderManager::boneShader;
        if (!activeShader || !activeShader->isCompiled()) {
            Logger::log("WARNING: Bone shader failed! Falling back to default shader.", Logger::WARNING);
            activeShader = ShaderManager::lightingShader ? ShaderManager::lightingShader : nullptr;
        }

        if (!activeShader) {
            Logger::log("ERROR: No valid shader available! Skipping rendering.", Logger::ERROR);
            return;  //  Prevents crash if no shader is available
        }

        Logger::log("DEBUG: Using active shader.", Logger::INFO);
        activeShader->use();

        activeShader->setMat4("view", camera.GetViewMatrix());
        activeShader->setMat4("projection", camera.ProjectionMatrix);

        // Scale model for visibility
        glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        activeShader->setMat4("model", modelMatrix);

        // Draw the model
        myModel->Draw(*activeShader);
        Logger::log("DEBUG: Model drawn successfully.", Logger::INFO);

        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
