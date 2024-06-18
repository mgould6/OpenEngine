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

void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer) {
    // 1. Extract bright areas
    glBindFramebuffer(GL_FRAMEBUFFER, bloomBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    brightExtractShader.use();
    brightExtractShader.setInt("scene", 0);
    brightExtractShader.setFloat("exposure", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrBuffer);
    renderQuad();

    // 2. Apply Gaussian blur
    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10;
    blurShader.use();
    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        blurShader.setInt("horizontal", horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? bloomBuffer : pingpongBuffer[!horizontal]);
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }

    // 3. Combine with original scene
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    combineShader.use();
    combineShader.setInt("scene", 0);
    combineShader.setInt("bloomBlur", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrBuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
    renderQuad();
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

    // Load bloom shaders
    Shader brightExtractShader("bright_extract.vs", "bright_extract.fs");
    Shader blurShader("blur.vs", "blur.fs");
    Shader combineShader("combine.vs", "combine.fs");

    // Initialize depth map FBO
    unsigned int depthMapFBO, depthMap;
    initDepthMapFBO(depthMapFBO, depthMap);

    // HDR Framebuffer
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // Create HDR color buffer
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    // Create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        Logger::log("Framebuffer not complete!", Logger::ERROR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ping-pong framebuffers for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongBuffer[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            Logger::log("Pingpong framebuffer not complete!", Logger::ERROR);
    }

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

        // 2. Render scene as normal with shadow mapping to the HDR framebuffer
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Ensure the viewport is set to the whole window size
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ShaderManager::lightingShader->use();
        setUniforms(*ShaderManager::lightingShader);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        renderScene(*ShaderManager::lightingShader, VAO);
        checkGLError("Scene Rendering");

        // 3. Apply bloom effect
        applyBloomEffect(brightExtractShader, blurShader, combineShader, colorBuffers[0], colorBuffers[1], pingpongFBO, pingpongBuffer);

        // 4. Debug: render the depth map to a small quad on the screen
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
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteFramebuffers(1, &depthMapFBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
