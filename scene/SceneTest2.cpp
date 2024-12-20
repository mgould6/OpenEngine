#include "SceneTest2.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../common_utils/Logger.h"
#include "../input/InputManager.h"
#include "../model/Camera.h"
#include "../shaders/ShaderManager.h"
#include "../setup/Setup.h"
#include "../render_utils/Renderer.h"
#include "../cleanup/Cleanup.h"
#include "../setup/Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstddef>
#include "../setup/GraphicsSetup.h"
#include "../setup/InputCallbacks.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

extern unsigned int cubeVAO, cubeVBO;
extern btRigidBody* cube;
extern Model* myModel;

void SceneTest2(GLFWwindow* window) {
    setupFramebuffers();
    registerInputCallbacks();

    // Adjust the camera for viewing the character model
    camera = Camera(glm::vec3(0.0f, 2.0f, 10.0f));
    InputManager::setCamera(&camera);

    Renderer::InitializeImGui(window);

    // Load the character model using Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("Character Template F.fbx",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::log("Failed to load model: " + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    // Load the model into a custom Model class for rendering
    myModel = new Model("Character Template F.fbx");

    Logger::log("Successfully loaded Character Template F.fbx", Logger::INFO);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);
        camera.MovementSpeed = Renderer::getCameraSpeed();

        Renderer::BeginFrame();

        // Use the appropriate shader
        Shader* activeShader = ShaderManager::lightingShader;
        if (activeShader) {
            myModel->Draw(*activeShader);
        }
        else {
            Logger::log("Shader not initialized.", Logger::ERROR);
        }

        Renderer::EndFrame(window);
    }

    // Clean up
    delete myModel;
    Renderer::ShutdownImGui();
    cleanupResources();
}
