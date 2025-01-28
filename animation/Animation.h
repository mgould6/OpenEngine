#ifndef ANIMATION_H
#define ANIMATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../model/Model.h"

struct Keyframe {
    float timestamp;
    std::unordered_map<std::string, glm::mat4> boneTransforms;
};

class Animation {
public:
    explicit Animation(const std::string& filePath);

    bool isLoaded() const;
    float getDuration() const;

    void apply(float animationTime, Model* model);

private:
    std::string filePath;
    bool loaded;
    float duration;
    std::vector<Keyframe> keyframes;

    void loadAnimation(const std::string& filePath);

    // Update: Correct the function signature
    glm::mat4 interpolateKeyframes(const glm::mat4& transform1, const glm::mat4& transform2, float factor);
};

#endif // ANIMATION_H
