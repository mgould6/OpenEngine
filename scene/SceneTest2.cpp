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
        myModel = new Model("Character Template F3.fbx");
        if (!myModel) {
            Logger::log("Failed to load model.", Logger::ERROR);
            return;
        }
        Logger::log("Model loaded successfully.", Logger::INFO);
    }

    // Debug: Print absolute path of animation file
    std::string fullPath = std::filesystem::absolute("Character Template F3.fbx").string();
    Logger::log("Debug: Full path of animation file: " + fullPath, Logger::INFO);

    // Initialize the animation controller
    if (!animationController) {
        animationController = new AnimationController(myModel);
        if (!animationController->loadAnimation("Idle", "Character Template F3.fbx")) {
            Logger::log("Failed to load idle animation.", Logger::ERROR);
            return;
        }
        Logger::log("Idle animation loaded successfully.", Logger::INFO);
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

        if (animationController) {
            Logger::log("Debug: Calling animation update", Logger::INFO);
            animationController->update(deltaTime);
            Logger::log("Debug: Calling applyToModel", Logger::INFO);
            animationController->applyToModel(myModel);
        }

        Shader* activeShader = ShaderManager::boneShader ? ShaderManager::boneShader : ShaderManager::lightingShader;
        Logger::log("Debug: Using bone shader.", Logger::INFO);

        activeShader->use();
        activeShader->setMat4("view", camera.GetViewMatrix());
        activeShader->setMat4("projection", camera.ProjectionMatrix);
        activeShader->setMat4("model", glm::mat4(1.0f));

        myModel->Draw(*activeShader);

        DebugTools::renderBoneHierarchy(myModel, camera);
        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}
