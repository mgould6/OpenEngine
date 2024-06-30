#include "Renderer.h"
#include "shaders/ShaderManager.h"
#include "model/Camera.h"
#include "model/Model.h"
#include "render_utils/RenderUtils.h"
#include "common_utils/Logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

// External declarations
extern Camera camera;
extern Model* myModel;
extern unsigned int depthMapFBO, hdrFBO, depthMap;
extern unsigned int colorBuffers[2], pingpongFBO[2], pingpongBuffer[2];
extern float lastFrame;
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern unsigned int VAO;

void setUniforms(const Shader& shader) {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.ProjectionMatrix;

    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
    glm::mat4 lightView = glm::lookAt(glm::vec3(2.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader.setVec3("viewPos", camera.Position);
    shader.setVec3("lightPos", glm::vec3(2.0f, 4.0f, 2.0f));
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

void render(GLFWwindow* window, float deltaTime) {
    // 1. Render depth map
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    ShaderManager::depthShader->use();
    setUniforms(*ShaderManager::depthShader);
    renderScene(*ShaderManager::depthShader, VAO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Render scene with lighting and shadows
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ShaderManager::lightingShader->use();
    setUniforms(*ShaderManager::lightingShader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    renderScene(*ShaderManager::lightingShader, VAO);

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
    glBindTexture(GL_TEXTURE_2D, depthMap);
    renderQuad();
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
