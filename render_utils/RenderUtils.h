#pragma once

#include "../shaders/Shader.h"

void renderScene(const Shader& shader, unsigned int VAO);
void renderQuad();
void setUniforms(const Shader& shader);
void applyBloomEffect(const Shader& brightExtractShader, const Shader& blurShader, const Shader& combineShader, unsigned int hdrBuffer, unsigned int bloomBuffer, unsigned int* pingpongFBO, unsigned int* pingpongBuffer);
