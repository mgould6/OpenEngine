#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../common_utils/Logger.h"
#include "../model/Camera.h"
#include "../model/Model.h"
#include "../shaders/ShaderManager.h"
#include <random>
#include "../setup/Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "../Animation/AnimationController.h"

// Static variable definitions
const int NUM_CASCADES = Renderer::NUM_CASCADES;
float Renderer::cascadeSplits[NUM_CASCADES] = { 0.1f, 0.3f, 0.6f, 1.0f };
unsigned int Renderer::depthMapFBO[NUM_CASCADES];
unsigned int Renderer::depthMap[NUM_CASCADES];
glm::mat4 Renderer::lightSpaceMatrices[NUM_CASCADES];

unsigned int Renderer::ssaoFBO;
unsigned int Renderer::ssaoColorBuffer;
unsigned int Renderer::noiseTexture;
std::vector<glm::vec3> Renderer::ssaoKernel;

unsigned int Renderer::gPositionDepth;
unsigned int Renderer::gNormal;

LightManager Renderer::lightManager;

float Renderer::cameraSpeed = SPEED;
float Renderer::lightIntensity = 1.0f;
PhysicsManager Renderer::physicsManager;

unsigned int Renderer::shadowMapTexture = 0;

// Implementation for renderScene
void Renderer::renderScene(const Shader& shader, unsigned int VAO) {
    shader.use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // Adjust vertex count if needed
    glBindVertexArray(0);
}

// Implementation for setUniforms
void Renderer::setUniforms(const Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos) {
    glm::mat4 model = glm::mat4(1.0f);

    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("viewPos", viewPos);

}

// Function Definitions
void Renderer::createFramebuffer(unsigned int& framebuffer, unsigned int& texture, int width, int height, GLenum format) {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::log("Framebuffer not complete!", Logger::ERROR);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::createPingPongFramebuffers(unsigned int* pingpongFBO, unsigned int* pingpongBuffer, int width, int height) {
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::log("Pingpong framebuffer not complete!", Logger::ERROR);
        }
    }
}

void setLightUniforms(const Shader& shader) {
    shader.use();
    shader.setFloat("lightIntensity", Renderer::getLightIntensity());

    for (int i = 0; i < Renderer::lightManager.getDirectionalLights().size(); ++i) {
        const auto& light = Renderer::lightManager.getDirectionalLights()[i];
        shader.setVec3("dirLights[" + std::to_string(i) + "].direction", light.direction);
        shader.setVec3("dirLights[" + std::to_string(i) + "].ambient", light.ambient * Renderer::getLightIntensity());
        shader.setVec3("dirLights[" + std::to_string(i) + "].diffuse", light.diffuse * Renderer::getLightIntensity());
        shader.setVec3("dirLights[" + std::to_string(i) + "].specular", light.specular * Renderer::getLightIntensity());
    }
    for (int i = 0; i < Renderer::lightManager.getPointLights().size(); ++i) {
        const auto& light = Renderer::lightManager.getPointLights()[i];
        shader.setVec3("pointLights[" + std::to_string(i) + "].position", light.position);
        shader.setVec3("pointLights[" + std::to_string(i) + "].ambient", light.ambient * Renderer::getLightIntensity());
        shader.setVec3("pointLights[" + std::to_string(i) + "].diffuse", light.diffuse * Renderer::getLightIntensity());
        shader.setVec3("pointLights[" + std::to_string(i) + "].specular", light.specular * Renderer::getLightIntensity());
        shader.setFloat("pointLights[" + std::to_string(i) + "].constant", light.constant);
        shader.setFloat("pointLights[" + std::to_string(i) + "].linear", light.linear);
        shader.setFloat("pointLights[" + std::to_string(i) + "].quadratic", light.quadratic);
    }
    for (int i = 0; i < Renderer::lightManager.getSpotlights().size(); ++i) {
        const auto& light = Renderer::lightManager.getSpotlights()[i];
        shader.setVec3("spotlights[" + std::to_string(i) + "].position", light.position);
        shader.setVec3("spotlights[" + std::to_string(i) + "].direction", light.direction);
        shader.setVec3("spotlights[" + std::to_string(i) + "].ambient", light.ambient * Renderer::getLightIntensity());
        shader.setVec3("spotlights[" + std::to_string(i) + "].diffuse", light.diffuse * Renderer::getLightIntensity());
        shader.setVec3("spotlights[" + std::to_string(i) + "].specular", light.specular * Renderer::getLightIntensity());
        shader.setFloat("spotlights[" + std::to_string(i) + "].cutOff", light.cutOff);
        shader.setFloat("spotlights[" + std::to_string(i) + "].outerCutOff", light.outerCutOff);
        shader.setFloat("spotlights[" + std::to_string(i) + "].constant", light.constant);
        shader.setFloat("spotlights[" + std::to_string(i) + "].linear", light.linear);
        shader.setFloat("spotlights[" + std::to_string(i) + "].quadratic", light.quadratic);
    }
}
std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
    const glm::mat4 inv = glm::inverse(proj * view);
    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = inv * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}

bool Renderer::initShadowMapping() {
    glGenFramebuffers(NUM_CASCADES, depthMapFBO);
    glGenTextures(NUM_CASCADES, depthMap);

    for (int i = 0; i < NUM_CASCADES; ++i) {
        glBindTexture(GL_TEXTURE_2D, depthMap[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::log("Framebuffer not complete!", Logger::ERROR);
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

std::vector<glm::mat4> Renderer::getLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::vec3& lightDir) {
    float lastSplitDist = 0.0f;
    std::vector<glm::mat4> matrices;
    for (int i = 0; i < NUM_CASCADES; ++i) {
        float splitDist = cascadeSplits[i];
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), SCR_WIDTH / (float)SCR_HEIGHT, lastSplitDist, splitDist);
        std::vector<glm::vec4> frustumCorners = getFrustumCornersWorldSpace(proj, viewMatrix);

        glm::mat4 lightView = glm::lookAt(-lightDir * 500.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightProjection = computeLightProjection(frustumCorners, lightView);

        lightSpaceMatrices[i] = lightProjection * lightView;
        matrices.push_back(lightSpaceMatrices[i]);
        lastSplitDist = splitDist;
    }
    return matrices;
}

glm::mat4 Renderer::computeLightProjection(const std::vector<glm::vec4>& frustumCorners, const glm::mat4& lightView) {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : frustumCorners) {
        const glm::vec3 trf = glm::vec3(lightView * corner);
        min = glm::min(min, trf);
        max = glm::max(max, trf);
    }

    return glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
}
void Renderer::renderSceneWithShadows() {
    glm::vec3 lightDir = glm::vec3(0.5f, 1.0f, 0.3f);

    // Calculate light-space matrices
    std::vector<glm::mat4> lightSpaceMatrices = getLightSpaceMatrices(camera.GetViewMatrix(), lightDir);

    // Shadow pass
    for (int i = 0; i < NUM_CASCADES; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glViewport(0, 0, 1024, 1024);
        glClear(GL_DEPTH_BUFFER_BIT);
        ShaderManager::depthShader->use();
        ShaderManager::depthShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);
        renderScene(*ShaderManager::depthShader, VAO);
    }

    // Reset framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Lighting pass
    ShaderManager::lightingShader->use();
    ShaderManager::lightingShader->setFloat("lightIntensity", Renderer::getLightIntensity());

    for (int i = 0; i < NUM_CASCADES; ++i) {
        ShaderManager::lightingShader->setMat4("lightSpaceMatrix[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
        ShaderManager::lightingShader->setFloat("cascadeSplits[" + std::to_string(i) + "]", cascadeSplits[i]);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, depthMap[i]);
    }
    renderScene(*ShaderManager::lightingShader, VAO);
}

bool Renderer::initSSAO() {
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;
        scale = 0.1f + scale * scale * (1.0f - 0.1f);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}
void Renderer::renderSSAO() {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    ShaderManager::ssaoShader->use();
    for (unsigned int i = 0; i < 64; ++i)
        ShaderManager::ssaoShader->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);

    ShaderManager::ssaoShader->setMat4("projection", camera.ProjectionMatrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Renderer::gPositionDepth);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Renderer::gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderLightingWithSSAO(unsigned int ssaoColorBuffer) {
    ShaderManager::lightingShader->use();
    ShaderManager::lightingShader->setInt("gPositionDepth", 0);
    ShaderManager::lightingShader->setInt("gNormal", 1);
    ShaderManager::lightingShader->setInt("ssao", 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPositionDepth);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);

    renderQuad();
}

void Renderer::render(GLFWwindow* window, float deltaTime) {
    std::cout << "Renderer::render - Start" << std::endl;

    setLightUniforms(*ShaderManager::lightingShader);

    renderSceneWithShadows();

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ShaderManager::lightingShader->use();

    // Define and calculate required matrices and vectors
    glm::mat4 view = camera.GetViewMatrix(); // Ensure `camera` is accessible and initialized
    glm::mat4 projection = camera.ProjectionMatrix;
    glm::vec3 viewPos = camera.Position;

    Renderer::setUniforms(*ShaderManager::lightingShader, view, projection, viewPos);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap[0]);

    applyBloomEffect(*ShaderManager::brightExtractShader, *ShaderManager::blurShader, *ShaderManager::combineShader,
        colorBuffers[0], colorBuffers[1], pingpongFBO, pingpongBuffer);

    renderSSAO();
    renderLightingWithSSAO(ssaoColorBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ShaderManager::toneMappingShader->use();
    ShaderManager::toneMappingShader->setFloat("exposure", 1.0f);
    ShaderManager::toneMappingShader->setFloat("gamma", 2.2f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    renderQuad();

    Renderer::RenderImGui();
    glfwSwapBuffers(window);
    glfwPollEvents();

    physicsManager.Update(deltaTime);

    std::cout << "Renderer::render - End" << std::endl;
}


void renderQuad() {
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;
    if (quadVAO == 0) {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}


void Renderer::applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader,
    unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    brightExtractShader.use();
    brightExtractShader.setInt("scene", 0);
    brightExtractShader.setFloat("exposure", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrBuffer);
    renderQuad();

    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10; // Adjust blur passes if needed
    blurShader.use();
    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        blurShader.setInt("horizontal", horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? bloomBuffer : pingpongBuffer[!horizontal]);
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

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

void Renderer::renderQuad() {
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;

    if (quadVAO == 0) {
        float quadVertices[] = {
            // Positions          // Texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}


void Renderer::InitializeImGui(GLFWwindow* window) {
    if (ImGui::GetCurrentContext() != nullptr) {
        Logger::log("ImGui already initialized. Skipping initialization.", Logger::WARNING);
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        Logger::log("Failed to initialize ImGui for GLFW.", Logger::ERROR);
        return;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) { // Adjust GLSL version if necessary
        Logger::log("Failed to initialize ImGui for OpenGL3.", Logger::ERROR);
        return;
    }

    Logger::log("ImGui initialized successfully.", Logger::INFO);
}
// -----------------------------------------------------------------------------
//  Renderer::RenderImGui
//  Draw a combo box, reload a clip the first time it’s requested, bind it,
//  and push the first-frame pose so there’s no visible stall.
// -----------------------------------------------------------------------------
void Renderer::RenderImGui()
{
    if (!ImGui::GetCurrentContext())
        return;

    extern Model* myModel;                       // <- already declared elsewhere
    extern AnimationController* animationController;

    ImGui::Begin("Animation Controller");

    /* 1. Clip names and file paths */
    static const char* animNames[] = { "Idle", "Jab_Head", "Stance1" };
    static const char* animFiles[] = {
        "animations/Jab_Head.fbx"
        "animations/Stance1.fbx",
        "animations/Idle.fbx",
    };

    /* 2. Track current vs. previous selection */
    static int currentIndex = 0;
    static int oldIndex = currentIndex;

    /* 3. Combo box – user pick updates currentIndex */
    ImGui::Combo("Animation",
        &currentIndex,
        animNames,
        IM_ARRAYSIZE(animNames));

    /* 4. If user picked a new clip this frame… */
    if (currentIndex != oldIndex)
    {
        const char* clipName = animNames[currentIndex];
        const char* clipPath = animFiles[currentIndex];

        /* (a) Load only if we haven’t loaded this clip before */
        if (!animationController->isClipLoaded(clipName))
        {
            if (!animationController->loadAnimation(clipName, clipPath, false))
            {
                Logger::log("Failed to load " + std::string(clipPath),
                    Logger::ERROR);
                ImGui::End();
                return;
            }
        }

        /* (b) Bind it for immediate playback */
        animationController->setCurrentAnimation(clipName);

        /* (c) Push first frame pose so no one-frame pop */
        animationController->update(0.0f);
        animationController->applyToModel(myModel);   // <- use the real model ptr

        Logger::log("NOW PLAYING: " + std::string(clipName), Logger::INFO);
    }

    /* 5. Playback Controls */
    ImGui::Separator();
    ImGui::Text("Playback Options:");
    ImGui::Checkbox("Play", &animationController->debugPlay);
    ImGui::Checkbox("Step Frame", &animationController->debugStep);
    ImGui::Checkbox("Rewind", &animationController->debugRewind);
    ImGui::Checkbox("Loop Playback", &animationController->loopPlayback); // <-- NEW!

    ImGui::Text("Current Frame: %d", animationController->debugFrame);

    /* 6. store selection for next frame AFTER comparison */
    oldIndex = currentIndex;

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void Renderer::ShutdownImGui() {
    if (!ImGui::GetCurrentContext()) {
        Logger::log("ImGui context is not initialized. Skipping shutdown.", Logger::WARNING);
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Logger::log("ImGui shutdown completed.", Logger::INFO);
}

float Renderer::getCameraSpeed() {
    return cameraSpeed;
}

void Renderer::setCameraSpeed(float speed) {
    cameraSpeed = speed;
}

float Renderer::getLightIntensity() {
    return lightIntensity;
}

void Renderer::setLightIntensity(float intensity) {
    lightIntensity = intensity;
}


void Renderer::BeginFrame() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Logger::log("DEBUG: Depth testing and blending enabled.", Logger::INFO);
}


void Renderer::EndFrame(GLFWwindow* window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
}
