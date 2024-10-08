#ifndef ENGINE_H
#define ENGINE_H

#include "../shaders/Shader.h"
#include "../light/LightManager.h"
#include "../model/Camera.h"

void initDepthMapFBO(unsigned int& depthMapFBO, unsigned int& depthMap);
void initPostProcessingFBO(unsigned int& postProcessingFBO, unsigned int& textureColorbuffer, unsigned int& rbo);
void renderDepthMap(const Shader& shader, unsigned int VAO, unsigned int depthMapFBO);
void renderScene(const Shader& shader, unsigned int VAO);
void applyPostProcessing(const Shader& shader, unsigned int textureColorbuffer);
void initializeLights(); // Add function prototype to initialize lights

#endif // ENGINE_H
