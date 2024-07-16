#include "LightManager.h"

void LightManager::addDirectionalLight(const DirectionalLight& light) {
    if (directionalLights.size() < MAX_LIGHTS) {
        directionalLights.push_back(light);
    }
}

void LightManager::addPointLight(const PointLight& light) {
    if (pointLights.size() < MAX_LIGHTS) {
        pointLights.push_back(light);
    }
}

void LightManager::addSpotlight(const Spotlight& light) {
    if (spotlights.size() < MAX_LIGHTS) {
        spotlights.push_back(light);
    }
}

std::vector<DirectionalLight> LightManager::getDirectionalLights() const {
    return directionalLights;
}

std::vector<PointLight> LightManager::getPointLights() const {
    return pointLights;
}

std::vector<Spotlight> LightManager::getSpotlights() const {
    return spotlights;
}
