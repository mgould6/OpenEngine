#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <glm/glm.hpp>
#include "../shaders/Shader.h"
#include <GLFW/glfw3.h>
#include "../light/LightManager.h"
#include "../physics/PhysicsManager.h"

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern unsigned int cubeVAO, planeVAO;

class Renderer {
public:
    static const int NUM_CASCADES = 4;
    static bool initShadowMapping();
    static void renderSceneWithShadows();
    static std::vector<glm::mat4> getLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::vec3& lightDir);
    static glm::mat4 computeLightProjection(const std::vector<glm::vec4>& frustumCorners, const glm::mat4& lightView);

    static float cascadeSplits[NUM_CASCADES];
    static unsigned int depthMapFBO[NUM_CASCADES];
    static unsigned int depthMap[NUM_CASCADES];
    static glm::mat4 lightSpaceMatrices[NUM_CASCADES];

    static bool initSSAO();
    static void renderSSAO();
    static void renderLightingWithSSAO(unsigned int ssaoColorBuffer);

    static unsigned int ssaoFBO, ssaoColorBuffer, noiseTexture;
    static unsigned int gPositionDepth, gNormal;
    static std::vector<glm::vec3> ssaoKernel;

    static void render(GLFWwindow* window, float deltaTime);

    static LightManager lightManager;
    static PhysicsManager physicsManager; // Moved PhysicsManager to public

    static void InitializeImGui(GLFWwindow* window);
    static void RenderImGui();
    static void ShutdownImGui();

    static float getCameraSpeed();
    static void setCameraSpeed(float speed);

    static float getLightIntensity();
    static void setLightIntensity(float intensity);

    static void createFramebuffer(unsigned int& framebuffer, unsigned int& texture, int width, int height, GLenum format);
    static void createPingPongFramebuffers(unsigned int* pingpongFBO, unsigned int* pingpongBuffer, int width, int height);

private:
    static float cameraSpeed;
    static float lightIntensity;
};

void setUniforms(const Shader& shader);
void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer);
void renderScene(const Shader& shader, unsigned int VAO);
void renderQuad();
void renderCube(const Shader& shader);
void renderPlane(const Shader& shader);


#endif // RENDERER_H
