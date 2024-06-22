#include "CascadedShadowMap.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>  // Include iostream for cerr and endl

CascadedShadowMap::CascadedShadowMap() {
    lightSpaceMatrices.resize(NUM_CASCADES);
    cascadeEnds.resize(NUM_CASCADES + 1);
}

void CascadedShadowMap::init(unsigned int width, unsigned int height) {
    glGenFramebuffers(NUM_CASCADES, depthMapFBOs);
    glGenTextures(NUM_CASCADES, depthMaps);

    for (int i = 0; i < NUM_CASCADES; ++i) {
        glBindTexture(GL_TEXTURE_2D, depthMaps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMaps[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Framebuffer not complete for cascade " << i << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void CascadedShadowMap::setLightSpaceMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDir) {
    updateLightSpaceMatrices(view, projection, lightDir);
}

void CascadedShadowMap::updateLightSpaceMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDir) {
    // Implement cascade calculation here
    // Update lightSpaceMatrices and cascadeEnds
}
