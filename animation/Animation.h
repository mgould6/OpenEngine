#ifndef ANIMATION_H
#define ANIMATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <map>  
#include <glm/glm.hpp>
#include "../model/Model.h"

struct Keyframe {
    float timestamp;
    std::map<std::string, glm::mat4> boneTransforms;
};

class Animation {
public:
    explicit Animation(const std::string& filePath);

    bool isLoaded() const;
    float getDuration() const;
    float getTicksPerSecond() const { return ticksPerSecond; }

    void apply(float animationTime, Model* model);
    void interpolateKeyframes(float animationTime, std::map<std::string, glm::mat4>& finalBoneMatrices) const;

private:
    std::string filePath;
    bool loaded;
    float duration;
    float ticksPerSecond;
    std::vector<Keyframe> keyframes;

    void loadAnimation(const std::string& filePath);
    glm::mat4 interpolateKeyframes(const glm::mat4& transform1, const glm::mat4& transform2, float factor) const;

    std::vector<std::string> animatedBones;  // Store bone names that exist in this animation

};

#endif // ANIMATION_H
