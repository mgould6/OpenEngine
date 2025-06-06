// SceneTest3.cpp
#include "SceneTest3.h"
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
#include <glm/gtx/string_cast.hpp>

void SceneTest3(GLFWwindow* window) {
    // Global variables
    Camera camera;
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    Model* myModel = nullptr;
    AnimationController* animationController = nullptr;

    Logger::log("Entering SceneTest3 for animation system expansion testing.", Logger::INFO);

    // Load the model
    myModel = new Model("CharacterModel.fbx");
    if (!myModel) {
        Logger::log("ERROR: Failed to load model.", Logger::ERROR);
        return;
    }
    Logger::log("INFO: Model loaded successfully.", Logger::INFO);
    camera.setCameraToFitModel(*myModel);

    // Initialize the animation controller
    animationController = new AnimationController(myModel);
    animationController->loadAnimation("Stance1", "animations/Stance1.fbx");
    animationController->loadAnimation("Jab_Head", "animations/Jab_Head.fbx");
    animationController->setCurrentAnimation("Stance1");
    Logger::log("INFO: Set current animation to Stance1.", Logger::INFO);
    animationController->update(0.0f);           // Apply T=0
    animationController->applyToModel(myModel);  // Force first-frame visuals

    // Sync globals for ImGui to access
    ::myModel = myModel;
    ::animationController = animationController;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        Renderer::BeginFrame();

        // Update and apply animation
        if (animationController) {
            animationController->update(deltaTime);
            animationController->applyToModel(myModel);
        }

        // Shader setup
        Shader* activeShader = ShaderManager::boneShader;
        if (!activeShader || !activeShader->isCompiled()) {
            Logger::log("WARNING: Bone shader failed! Falling back to lighting shader.", Logger::WARNING);
            activeShader = ShaderManager::lightingShader;
        }

        activeShader->use();
        activeShader->setMat4("view", camera.GetViewMatrix());
        activeShader->setMat4("projection", camera.ProjectionMatrix);

        glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        activeShader->setMat4("model", modelMatrix);
        activeShader->setMat4Array("boneTransforms", myModel->getFinalBoneMatrices());

        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        myModel->Draw(*activeShader);

        Renderer::RenderImGui(); // All GUI logic is handled here
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest3.", Logger::INFO);
}
