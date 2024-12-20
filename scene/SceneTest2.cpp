#include "SceneTest2.h"
#include "../setup/GraphicsSetup.h"
#include "../setup/InputCallbacks.h"
#include "../model/Camera.h"
#include "../setup/Globals.cpp"
#include "../input/InputManager.h"


// A simple test 2 - 
void SceneTest2(GLFWwindow* window) {
    setupFramebuffers();
    setupCubeVertexData();
    setupPlaneVertexData();
    registerInputCallbacks();

    // Adjust the camera to view the scene with the cube and plane
    camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f)); // Positioned to view the cube dropping onto the plane
    InputManager::setCamera(&camera);

    Renderer::InitializeImGui(window);
    Renderer::physicsManager.Initialize(); // Initialize PhysicsManager

}
