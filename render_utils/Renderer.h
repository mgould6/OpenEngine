#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <glm/glm.hpp>
#include "../light/LightManager.h"
#include "../physics/PhysicsManager.h"
#include <GLFW/glfw3.h> // For GLFWwindow
#include <glad/glad.h>  // For GLenum
#include "../shaders/Shader.h"

class Renderer {
public:
    static const int NUM_CASCADES = 4;
    static float cascadeSplits[NUM_CASCADES];
    static unsigned int depthMapFBO[NUM_CASCADES];
    static unsigned int depthMap[NUM_CASCADES];
    static glm::mat4 lightSpaceMatrices[NUM_CASCADES];
    static unsigned int shadowMapTexture;

    static unsigned int ssaoFBO;
    static unsigned int ssaoColorBuffer;
    static unsigned int noiseTexture;
    static std::vector<glm::vec3> ssaoKernel;

    static unsigned int gPositionDepth;
    static unsigned int gNormal;

    static LightManager lightManager;

    static float getCameraSpeed();
    static void setCameraSpeed(float speed);

    static float getLightIntensity();
    static void setLightIntensity(float intensity);

    static void createFramebuffer(unsigned int& framebuffer, unsigned int& texture, int width, int height, GLenum format);
    static void createPingPongFramebuffers(unsigned int* pingpongFBO, unsigned int* pingpongBuffer, int width, int height);
    static bool initShadowMapping();
    static std::vector<glm::mat4> getLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::vec3& lightDir);
    static glm::mat4 computeLightProjection(const std::vector<glm::vec4>& frustumCorners, const glm::mat4& lightView);
    static void renderScene(const Shader& shader, unsigned int VAO); // Fixed declaration
    static void renderSceneWithShadows();
    static bool initSSAO();
    static void renderSSAO();
    static void renderLightingWithSSAO(unsigned int ssaoColorBuffer);
    static void InitializeImGui(GLFWwindow* window);
    static void RenderImGui();
    static void ShutdownImGui();
    static void render(GLFWwindow* window, float deltaTime);
    static void BeginFrame();
    static void EndFrame(GLFWwindow* window);

private:
    static float cameraSpeed;
    static float lightIntensity;
    static PhysicsManager physicsManager;

    static void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader,
        unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer);
    static void renderQuad();

    static void setUniforms(const Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos);
};

#endif // RENDERER_H
