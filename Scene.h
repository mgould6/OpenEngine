// Scene.h
#pragma once

#include <vector>
#include "Shader.h"
#include "Camera.h"
#include <glm/glm.hpp>

class Scene {
public:
    void addObject(const glm::vec3& position);
    void render(Shader& shader, Camera& camera);

private:
    std::vector<glm::vec3> objects;
};

