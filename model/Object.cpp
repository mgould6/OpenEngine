#include "Object.h"
#include <glm/gtc/matrix_transform.hpp>

Object::Object()
    : position(0.0f), rotation(0.0f), scale(1.0f), modelMatrix(1.0f) {}

void Object::setPosition(const glm::vec3& position) {
    this->position = position;
    updateModelMatrix();
}

void Object::setRotation(const glm::vec3& rotation) {
    this->rotation = rotation;
    updateModelMatrix();
}

void Object::setScale(const glm::vec3& scale) {
    this->scale = scale;
    updateModelMatrix();
}

void Object::update(float deltaTime) {
    // Update object logic (e.g., animations)
}

void Object::render(const Shader& shader) const {
    shader.setMat4("model", modelMatrix);
    // Bind VAO and render the object
}

void Object::updateModelMatrix() {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, scale);
}
