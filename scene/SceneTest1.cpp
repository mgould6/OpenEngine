#include "SceneTest1.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../common_utils/Logger.h"
#include "../input/InputManager.h"
#include "../engine/Engine.h"
#include "../model/Camera.h"
#include "../shaders/ShaderManager.h"
#include "../setup/Setup.h"
#include "../render_utils/Renderer.h"
#include "../cleanup/Cleanup.h"
#include "../setup/Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstddef> // For std::size
#include "../setup/GraphicsSetup.h"
#include "../setup/InputCallbacks.h"

// Cube and Plane VAOs and VBOs
extern unsigned int cubeVAO, cubeVBO;
extern unsigned int planeVAO, planeVBO;

// Physics objects
extern btRigidBody* cube;
extern btRigidBody* ground;

void SceneTest1(GLFWwindow* window) {
    setupFramebuffers();
    setupCubeVertexData();
    setupPlaneVertexData();
    registerInputCallbacks();

    // Adjust the camera to view the scene with the cube and plane
    camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f)); // Positioned to view the cube dropping onto the plane
    InputManager::setCamera(&camera);

    Renderer::InitializeImGui(window);
    Renderer::physicsManager.Initialize(); // Initialize PhysicsManager

    // Create a plane
    ground = Renderer::physicsManager.CreatePlane(btVector3(0, 1, 0), 0);

    // Create a cube
    cube = Renderer::physicsManager.CreateCube(1.0f, 1.0f, btVector3(0, 10, 0));
}
