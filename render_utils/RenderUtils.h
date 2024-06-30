#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#include "../shaders/Shader.h"

void renderScene(const Shader& shader, unsigned int VAO);
void renderQuad(); // Make sure this is declared if it's used in your rendering functions

#endif
