#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>
#include "Shader.h"
#include "Camera.h"
#include "InputManager.h"
#include "Engine.h"
#include "ShaderManager.h"
#include "Logger.h"
#include "Utils.h"
#include "RenderUtils.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float deltaTime = 0.0f;  // Time between current frame and last frame
float lastFrame = 0.0f;  // Time of last frame

void printMatrix(const glm::mat4& matrix, const std::string& name) {
    std::cout << name << " Matrix:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << std::setw(10) << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

void setUniforms(const Shader& shader) {
    // Set uniform values
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
    glm::mat4 lightView = glm::lookAt(glm::vec3(1.2f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader.setVec3("viewPos", camera.Position);
    shader.setVec3("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
    shader.setInt("shadowMap", 1);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        Logger::log("Failed to initialize GLFW", Logger::ERROR);
        return -1;
    }

    // Set GLFW context version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFWwindow object
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Game Engine", NULL, NULL);
    if (window == NULL) {
        Logger::log("Failed to create GLFW window", Logger::ERROR);
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL function pointers using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::log("Failed to initialize GLAD", Logger::ERROR);
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, InputManager::mouseCallback);
    glfwSetScrollCallback(window, InputManager::scrollCallback);
    glfwSetMouseButtonCallback(window, InputManager::mouseButtonCallback);

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Initialize shaders
    ShaderManager::initShaders();

    // Initialize depth map FBO
    unsigned int depthMapFBO, depthMap;
    initDepthMapFBO(depthMapFBO, depthMap);

    // Post-Processing FBO
    unsigned int postProcessingFBO, textureColorbuffer, rbo;
    initPostProcessingFBO(postProcessingFBO, textureColorbuffer, rbo);

    // Setup vertex data and buffers
    float vertices[] = {
        // positions         // normals
        -0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.5f, 0.0f,  0.0f,  0.0f, 1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set the camera for InputManager
    InputManager::setCamera(&camera);

    // Register input callbacks
    InputManager::registerKeyCallback(GLFW_KEY_W, []() {
        camera.ProcessKeyboard(FORWARD, deltaTime);
        });

    InputManager::registerKeyCallback(GLFW_KEY_S, []() {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        });

    InputManager::registerKeyCallback(GLFW_KEY_A, []() {
        camera.ProcessKeyboard(LEFT, deltaTime);
        });

    InputManager::registerKeyCallback(GLFW_KEY_D, []() {
        camera.ProcessKeyboard(RIGHT, deltaTime);
        });

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputManager::processInput(window, deltaTime);

        // 1. Render depth of scene to texture (from light's perspective)
        glViewport(0, 0, 1024, 1024);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        ShaderManager::depthShader->use();
        setUniforms(*ShaderManager::depthShader);
        renderScene(*ShaderManager::depthShader, VAO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        checkGLError("Depth Map Rendering");

        // 2. Render scene as normal with shadow mapping to the post-processing framebuffer
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Ensure the viewport is set to the whole window size
        glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ShaderManager::lightingShader->use();
        setUniforms(*ShaderManager::lightingShader);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        renderScene(*ShaderManager::lightingShader, VAO);
        checkGLError("Scene Rendering");

        // 3. Apply post-processing effects
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Ensure the viewport is set to the whole window size
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderManager::postProcessingShader->use();
        ShaderManager::postProcessingShader->setInt("screenTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer); // Use the color attachment texture as the quad's texture
        renderQuad();
        checkGLError("Post-Processing");

        // Debug: render the depth map to a small quad on the screen
        glViewport(0, 0, SCR_WIDTH / 4, SCR_HEIGHT / 4); // Bottom-left corner
        glDisable(GL_DEPTH_TEST);
        ShaderManager::debugDepthQuad->use();
        ShaderManager::debugDepthQuad->setInt("depthMap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        renderQuad();
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteFramebuffers(1, &postProcessingFBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
