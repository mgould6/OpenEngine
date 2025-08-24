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
#include <imgui.h>
#include <fstream>

void SceneTest3(GLFWwindow* window) {
    // Global variables
    Camera camera;
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    Model* myModel = nullptr;
    AnimationController* animationController = nullptr;

    Logger::log("Entering SceneTest3 for animation system expansion testing.", Logger::INFO);

    // Load the model
    myModel = new Model("CharacterModelTPose w shorts.fbx");
    if (!myModel) {
        Logger::log("ERROR: Failed to load model.", Logger::ERROR);
        return;
    }
    Logger::log("INFO: Model loaded successfully.", Logger::INFO);
    camera.setCameraToFitModel(*myModel);

    // Initialize the animation controller
    animationController = new AnimationController(myModel);
    animationController->loadAnimation("Jab_Head", "animations/Jab_Head.fbx");
    animationController->getAllAnimations().at("Jab_Head")->suppressPostBakeJitter(); // <- Right leg smoothing
    animationController->loadAnimation("Idle", "animations/Idle.fbx");
    animationController->loadAnimation("Stance1", "animations/Stance1.fbx");
    animationController->setCurrentAnimation("Jab_Head"); // <- Visually test smoothed animation
    animationController->loopPlayback = true;
    Logger::log("INFO: Set current animation to Jab_Head.", Logger::INFO);

 /*   std::ofstream clear("logs/pose_dump_engine_Jab_Head.log", std::ios::trunc);
    clear.close();*/

    animationController->update(0.0f);
    animationController->applyToModel(myModel);

    // Sync globals for ImGui to access
    ::myModel = myModel;
    ::animationController = animationController;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        Renderer::BeginFrame();

        // Update animation and apply
        if (animationController) {
            animationController->update(deltaTime);
            animationController->applyToModel(myModel);
        }

        // ImGui playback controls
        if (animationController) {
            ImGui::Begin("Animation Debug");
            ImGui::Text("Animation: %s", animationController->isAnimationPlaying()
                ? animationController->getCurrentAnimationName().c_str()
                : "None");

            ImGui::Checkbox("Play", &animationController->debugPlay);
            if (ImGui::Button("Step")) animationController->debugStep = true;
            if (ImGui::Button("Rewind")) animationController->debugRewind = true;

            const auto& keyframes = animationController->getKeyframes();
            if (!keyframes.empty()) {
                ImGui::SliderInt("Frame", &animationController->debugFrame, 0,
                    static_cast<int>(keyframes.size()) - 1, "%d");
            }
            ImGui::End();
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

        glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f),
            glm::radians(-90.0f),
            glm::vec3(1.0f, 0.0f, 0.0f));
        activeShader->setMat4("model", modelMatrix);
        activeShader->setMat4Array("boneTransforms", myModel->getFinalBoneMatrices());

        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if (myModel->getBones().size() > 100) {
            Logger::log("[ERROR] Model has " +
                std::to_string(myModel->getBones().size()) +
                " bones, but shader array boneTransforms[100] is too small.",
                Logger::ERROR);
        }

        myModel->Draw(*activeShader);

        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest3.", Logger::INFO);
}
