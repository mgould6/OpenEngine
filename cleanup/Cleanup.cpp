#include "Cleanup.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../model/Model.h"
#include "../render_utils/Renderer.h"

extern Model* myModel;
extern unsigned int VAO, VBO, hdrFBO, depthMapFBO[];

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
