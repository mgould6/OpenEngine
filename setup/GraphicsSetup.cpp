#include "GraphicsSetup.h"
#include "../shaders/ShaderManager.h"
#include "../render_utils/Renderer.h"
#include "../common_utils/Logger.h"
#include "Globals.h"
#include "../setup/Setup.h"

extern unsigned int planeVBO, cubeVBO;

bool initializeGraphics(GLFWwindow*& window) {
    if (!initialize(window, SCR_WIDTH, SCR_HEIGHT)) {
        return false;
    }

    if (!ShaderManager::initShaders()) {
        Logger::log("Failed to initialize shaders", Logger::ERROR);
        return false;
    }

    if (!Renderer::initShadowMapping()) {
        Logger::log("Failed to initialize shadow mapping", Logger::ERROR);
        return false;
    }

    if (!Renderer::initSSAO()) {
        Logger::log("Failed to initialize SSAO", Logger::ERROR);
        return false;
    }

    return true;
}

void setupFramebuffers() {
    Renderer::createFramebuffer(hdrFBO, colorBuffers[0], SCR_WIDTH, SCR_HEIGHT, GL_RGBA16F);
    Renderer::createPingPongFramebuffers(pingpongFBO, pingpongBuffer, SCR_WIDTH, SCR_HEIGHT);

    glGenTextures(1, &Renderer::shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, Renderer::shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void setupCubeVertexData() {
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void setupPlaneVertexData() {
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
