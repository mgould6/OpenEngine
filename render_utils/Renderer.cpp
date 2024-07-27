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
#include "../Globals.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
PhysicsManager Renderer::physicsManager; // Added PhysicsManager

extern Model* myModel;
extern unsigned int hdrFBO;
extern unsigned int colorBuffers[2], pingpongFBO[2], pingpongBuffer[2];
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern unsigned int VAO;

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

    std::vector<glm::mat4> lightSpaceMatrices = getLightSpaceMatrices(camera.GetViewMatrix(), lightDir);

    for (int i = 0; i < NUM_CASCADES; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glViewport(0, 0, 1024, 1024);
        glClear(GL_DEPTH_BUFFER_BIT);
        ShaderManager::depthShader->use();
        ShaderManager::depthShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);
        renderScene(*ShaderManager::depthShader, VAO);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ShaderManager::lightingShader->use();
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

    glfwMakeContextCurrent(window);

    std::cout << "Renderer::render - Set Light Uniforms" << std::endl;
    setLightUniforms(*ShaderManager::lightingShader);

    std::cout << "Renderer::render - Render Depth Map" << std::endl;
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, Renderer::depthMapFBO[0]);
    glClear(GL_DEPTH_BUFFER_BIT);
    ShaderManager::depthShader->use();
    setUniforms(*ShaderManager::depthShader);
    renderScene(*ShaderManager::depthShader, VAO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "Renderer::render - Render Scene with Lighting and Shadows" << std::endl;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ShaderManager::lightingShader->use();
    setUniforms(*ShaderManager::lightingShader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Renderer::depthMap[0]);
    renderScene(*ShaderManager::lightingShader, VAO);

    if (myModel) {
        myModel->Draw(*ShaderManager::lightingShader);
    }

    std::cout << "Renderer::render - Apply Bloom Effect" << std::endl;
    applyBloomEffect(*ShaderManager::brightExtractShader, *ShaderManager::blurShader, *ShaderManager::combineShader, colorBuffers[0], colorBuffers[1], pingpongFBO, pingpongBuffer);

    std::cout << "Renderer::render - Render SSAO" << std::endl;
    Renderer::renderSSAO();

    std::cout << "Renderer::render - Render Lighting with SSAO" << std::endl;
    Renderer::renderLightingWithSSAO(Renderer::ssaoColorBuffer);

    std::cout << "Renderer::render - Tone Mapping and Gamma Correction" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ShaderManager::toneMappingShader->use();
    ShaderManager::toneMappingShader->setFloat("exposure", 1.0f);
    ShaderManager::toneMappingShader->setFloat("gamma", 2.2f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    renderQuad();

    std::cout << "Renderer::render - Render ImGui" << std::endl;
    Renderer::RenderImGui();

    glfwSwapBuffers(window);
    glfwPollEvents();

    std::cout << "Renderer::render - End" << std::endl;

    physicsManager.Update(deltaTime); // Update physics simulation
}

void renderScene(const Shader& shader, unsigned int VAO) {
    shader.use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
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

void setUniforms(const Shader& shader) {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.ProjectionMatrix;

    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
    glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, 2.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader.setVec3("viewPos", camera.Position);
    shader.setVec3("lightPos", lightPos);
    shader.setInt("shadowMap", 1);
}

void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    brightExtractShader.use();
    brightExtractShader.setInt("scene", 0);
    brightExtractShader.setFloat("exposure", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrBuffer);
    renderQuad();

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

void Renderer::InitializeImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Renderer::RenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Parameters");

    static float lightIntensity = Renderer::getLightIntensity();
    ImGui::SliderFloat("Light Intensity", &lightIntensity, 0.0f, 10.0f);
    Renderer::setLightIntensity(lightIntensity);

    static float cameraSpeed = Renderer::getCameraSpeed();
    ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.0f, 10.0f);
    Renderer::setCameraSpeed(cameraSpeed);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
