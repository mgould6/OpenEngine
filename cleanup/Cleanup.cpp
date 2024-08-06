#include "Cleanup.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../model/Model.h"
#include "../render_utils/Renderer.h"
#include "../setup/Globals.h"

extern Model* myModel;
extern unsigned int planeVBO, cubeVBO, VBO, VAO, VBO, hdrFBO, depthMapFBO[];

void cleanup() {
    delete myModel;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteFramebuffers(1, &hdrFBO);
    for (int i = 0; i < Renderer::NUM_CASCADES; ++i) {
        glDeleteFramebuffers(1, &depthMapFBO[i]);
    }
    glfwTerminate();
}

void cleanupResources() {
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(2, colorBuffers);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongBuffer);
    glfwTerminate();
}