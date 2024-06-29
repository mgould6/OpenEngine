#pragma once

#include <glm/glm.hpp>
#include "../shaders/Shader.h"

class Object {
public:
    Object();
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation);
    void setScale(const glm::vec3& scale);

    void update(float deltaTime);
    void render(const Shader& shader) const;

private:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 modelMatrix;

    void updateModelMatrix();
};
