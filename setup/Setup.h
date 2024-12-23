#ifndef SETUP_H
#define SETUP_H

#include <GLFW/glfw3.h>

bool initialize(GLFWwindow*& window, const unsigned int width, const unsigned int height);
extern void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#endif // SETUP_H
