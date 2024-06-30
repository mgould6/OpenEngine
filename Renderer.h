#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>
#include "shaders/Shader.h"

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

void render(GLFWwindow* window, float deltaTime);
void setUniforms(const Shader& shader);
void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer);

#endif
