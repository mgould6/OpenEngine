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
#include "../animation/DebugTools.h"
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

    // Debug: Print absolute path of animation file
    std::string fullPath = std::filesystem::absolute("Character Template F9.fbx").string();
    Logger::log("DEBUG: Full path of animation file: " + fullPath, Logger::INFO);

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

    camera.setCameraToFitModel(*myModel);
    Logger::log("INFO: Camera positioned to frame the model.", Logger::INFO);

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

        if (animationController) {
            Logger::log("DEBUG: Calling animation update", Logger::INFO);
            animationController->update(deltaTime);
            Logger::log("DEBUG: Calling applyToModel", Logger::INFO);
            animationController->applyToModel(myModel);
        }

        Shader* activeShader = ShaderManager::boneShader ? ShaderManager::boneShader : ShaderManager::lightingShader;
        Logger::log("DEBUG: Using bone shader.", Logger::INFO);

        activeShader->use();
        activeShader->setMat4("view", camera.GetViewMatrix());
        activeShader->setMat4("projection", camera.ProjectionMatrix);
        activeShader->setMat4("model", glm::mat4(1.0f));

        myModel->Draw(*activeShader);
        Logger::log("DEBUG: Model drawn successfully.", Logger::INFO);

        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
