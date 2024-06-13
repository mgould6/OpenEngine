#pragma once

#include <vector>
#include "Shader.h"
#include "Object.h"

class Scene {
public:
    void addObject(const Object& object);
    void removeObject(int index);
    void updateObjects(float deltaTime);
    void render(Shader& shader);

private:
    std::vector<Object> objects;
};
