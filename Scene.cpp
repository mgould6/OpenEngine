// Scene.cpp
#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>

void Scene::addObject(const glm::vec3& position) {
    objects.push_back(position);
}

void Scene::render(Shader& shader, Camera& camera) {
    for (const auto& position : objects) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        shader.setMat4("model", model);

        // Render your object here
    }
}
