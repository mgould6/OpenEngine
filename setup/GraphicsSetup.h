#ifndef GRAPHICS_SETUP_H
#define GRAPHICS_SETUP_H

#include <GLFW/glfw3.h>

bool initializeGraphics(GLFWwindow*& window);
void setupFramebuffers();
void setupCubeVertexData();
void setupPlaneVertexData();

#endif // GRAPHICS_SETUP_H
