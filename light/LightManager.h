#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <glm/glm.hpp>
#include <vector>

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct Spotlight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

class LightManager {
public:
    static const int MAX_LIGHTS = 4;

    void addDirectionalLight(const DirectionalLight& light);
    void addPointLight(const PointLight& light);
    void addSpotlight(const Spotlight& light);

    std::vector<DirectionalLight> getDirectionalLights() const;
    std::vector<PointLight> getPointLights() const;
    std::vector<Spotlight> getSpotlights() const;

private:
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<Spotlight> spotlights;
};

#endif
