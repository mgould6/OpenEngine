#ifndef CASCADED_SHADOW_MAP_H
#define CASCADED_SHADOW_MAP_H

#include <glm/glm.hpp>
#include <vector>

class CascadedShadowMap {
public:
    static const int NUM_CASCADES = 4;

    CascadedShadowMap();
    void init(unsigned int width, unsigned int height);
    void setLightSpaceMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDir);

    std::vector<glm::mat4> getLightSpaceMatrices() const { return lightSpaceMatrices; }
    std::vector<float> getCascadeEnds() const { return cascadeEnds; }

    unsigned int getDepthMapFBO(int index) const { return depthMapFBOs[index]; }
    unsigned int getDepthMap(int index) const { return depthMaps[index]; }

private:
    unsigned int depthMapFBOs[NUM_CASCADES];
    unsigned int depthMaps[NUM_CASCADES];

    std::vector<glm::mat4> lightSpaceMatrices;
    std::vector<float> cascadeEnds;

    void updateLightSpaceMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDir);
};

#endif // CASCADED_SHADOW_MAP_H
