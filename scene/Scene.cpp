#include "Scene.h"

void Scene::addObject(const Object& object) {
    objects.push_back(object);
}

void Scene::removeObject(int index) {
    if (index >= 0 && index < objects.size()) {
        objects.erase(objects.begin() + index);
    }
}

void Scene::updateObjects(float deltaTime) {
    for (Object& object : objects) {
        object.update(deltaTime);
    }
}

void Scene::render(Shader& shader) {
    for (const Object& object : objects) {
        object.render(shader);
    }
}
