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
#include <glm/gtx/string_cast.hpp>

// Global variables
Camera camera;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
Model* myModel = nullptr;
AnimationController* animationController = nullptr;

void SceneTest2(GLFWwindow* window) {
    Logger::log("Entering SceneTest2.", Logger::INFO);

    // Load the model
    if (!myModel) {
        myModel = new Model("Character_TPose_NoAnim.fbx");
        if (!myModel) {
            Logger::log("ERROR: Failed to load model.", Logger::ERROR);
            return;
        }
        Logger::log("INFO: Model loaded successfully.", Logger::INFO);

        // Adjust camera to fit model
        camera.setCameraToFitModel(*myModel);

    }

    //// Initialize the animation controller
    //if (!animationController) {
    //    animationController = new AnimationController(myModel);
    //    if (!animationController->loadAnimation("rig.001|Idle", "Character Template F4.fbx")) {
    //        Logger::log("ERROR: AnimationController failed to load animation!", Logger::ERROR);
    //        return;
    //    }
    //    animationController->setCurrentAnimation("rig.001|Idle");
    //    Logger::log("INFO: Set current animation to rig.001|Idle.", Logger::INFO);
    //}

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        Renderer::BeginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (animationController) {
            animationController->update(deltaTime);
            animationController->applyToModel(myModel);
        }

        Shader* activeShader = ShaderManager::boneShader;
        if (!activeShader || !activeShader->isCompiled()) {
            Logger::log("WARNING: Bone shader failed! Falling back to lighting shader.", Logger::WARNING);
            activeShader = ShaderManager::lightingShader;
        }

        if (!activeShader) {
            Logger::log("ERROR: No valid shader available! Skipping rendering.", Logger::ERROR);
            return;
        }

        activeShader->use();
        activeShader->setMat4("view", camera.GetViewMatrix());
        activeShader->setMat4("projection", camera.ProjectionMatrix);

        glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        activeShader->setMat4("model", modelMatrix);

        // Matrix Logging
        Logger::log("Model matrix explicitly logged: " + glm::to_string(modelMatrix), Logger::INFO);
        Logger::log("View matrix explicitly logged: " + glm::to_string(camera.GetViewMatrix()), Logger::INFO);
        Logger::log("Projection matrix explicitly logged: " + glm::to_string(camera.ProjectionMatrix), Logger::INFO);
        glDisable(GL_CULL_FACE);  // Explicitly disable face culling
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Explicitly set polygon mode to filled


        // Ensure boneTransforms are freshly computed
        std::unordered_map<std::string, glm::mat4> localTransforms;
        std::unordered_map<std::string, glm::mat4> globalTransforms;

        for (const auto& bone : myModel->getBones()) {
            auto bindIt = myModel->boneLocalBindTransforms.find(bone.name);
            if (bindIt != myModel->boneLocalBindTransforms.end()) {
                localTransforms[bone.name] = bindIt->second;
            }
            else {
                localTransforms[bone.name] = glm::mat4(1.0f); // Fallback to identity if missing
            }
        }


        // Then explicitly compute global transforms
        for (const auto& bone : myModel->getBones()) {
            myModel->calculateBoneTransform(bone.name, localTransforms, globalTransforms);
        }

        // Now, update the bone transforms in the model
        for (const auto& pair : globalTransforms) {
            myModel->setBoneTransform(pair.first, pair.second);
        }


        myModel->forceTestBoneTransform();

        myModel->Draw(*activeShader);

        Renderer::RenderImGui();
        Renderer::EndFrame(window);
    }

    Logger::log("Exiting SceneTest2.", Logger::INFO);
}

