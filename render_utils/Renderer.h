#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <glm/glm.hpp>
#include "../shaders/Shader.h"
#include <GLFW/glfw3.h>

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

class Renderer {
public:
    static const int NUM_CASCADES = 4;
    static void initShadowMapping();
    static void renderSceneWithShadows();
    static std::vector<glm::mat4> getLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::vec3& lightDir);
    static glm::mat4 computeLightProjection(const std::vector<glm::vec4>& frustumCorners, const glm::mat4& lightView);

    static float cascadeSplits[NUM_CASCADES];
    static unsigned int depthMapFBO[NUM_CASCADES];
    static unsigned int depthMap[NUM_CASCADES];
    static glm::mat4 lightSpaceMatrices[NUM_CASCADES];

private:
    // Move any private members or methods here
};

void render(GLFWwindow* window, float deltaTime);
void setUniforms(const Shader& shader);
void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer);

#endif
