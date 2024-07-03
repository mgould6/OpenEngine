#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../common_utils/Logger.h"
#include "../model/Camera.h"
#include "../model/Model.h"
#include "../shaders/ShaderManager.h"
#include "../render_utils/RenderUtils.h"

const int NUM_CASCADES = Renderer::NUM_CASCADES;
float Renderer::cascadeSplits[NUM_CASCADES] = { 0.1f, 0.3f, 0.6f, 1.0f };
unsigned int Renderer::depthMapFBO[NUM_CASCADES];
unsigned int Renderer::depthMap[NUM_CASCADES];
glm::mat4 Renderer::lightSpaceMatrices[NUM_CASCADES];

extern Camera camera;
extern Model* myModel;
extern unsigned int hdrFBO;
extern unsigned int colorBuffers[2], pingpongFBO[2], pingpongBuffer[2];
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern unsigned int VAO;

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

void Renderer::initShadowMapping() {
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
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    // Get light direction
    glm::vec3 lightDir = glm::vec3(0.5f, 1.0f, 0.3f);

    // Compute light space matrices
    std::vector<glm::mat4> lightSpaceMatrices = getLightSpaceMatrices(camera.GetViewMatrix(), lightDir);

    // Render depth map for each cascade
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

void render(GLFWwindow* window, float deltaTime) {
    // 1. Render depth map
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, Renderer::depthMapFBO[0]);  // Correctly reference the first FBO in array
    glClear(GL_DEPTH_BUFFER_BIT);
    ShaderManager::depthShader->use();
    setUniforms(*ShaderManager::depthShader);
    renderScene(*ShaderManager::depthShader, VAO);  // Render the scene for depth map
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Render scene with lighting and shadows
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ShaderManager::lightingShader->use();
    setUniforms(*ShaderManager::lightingShader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Renderer::depthMap[0]);  // Correctly reference the first depth map in array
    renderScene(*ShaderManager::lightingShader, VAO);  // Render the scene with lighting and shadows

    // Draw the loaded model
    if (myModel) {
        myModel->Draw(*ShaderManager::lightingShader);
    }

    // 3. Apply bloom effect
    applyBloomEffect(*ShaderManager::brightExtractShader, *ShaderManager::blurShader, *ShaderManager::combineShader, colorBuffers[0], colorBuffers[1], pingpongFBO, pingpongBuffer);

    // 4. Visualize depth map for debugging
    glViewport(0, 0, SCR_WIDTH / 4, SCR_HEIGHT / 4);
    glDisable(GL_DEPTH_TEST);
    ShaderManager::debugDepthQuad->use();
    ShaderManager::debugDepthQuad->setInt("depthMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Renderer::depthMap[0]);  // Correctly reference the first depth map in array
    renderQuad();
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
