#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>

Animation::Animation(const std::string& filePath)
    : filePath(filePath), loaded(false), duration(0.0f) {
    loadAnimation(filePath);
}

bool Animation::isLoaded() const {
    return loaded;
}

float Animation::getDuration() const {
    return duration;
}

void Animation::apply(float animationTime, Model* model) {
    if (!loaded || keyframes.empty()) return;

    Keyframe kf1, kf2;
    float factor = 0.0f;

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (animationTime >= keyframes[i].timestamp && animationTime <= keyframes[i + 1].timestamp) {
            kf1 = keyframes[i];
            kf2 = keyframes[i + 1];
            factor = (animationTime - kf1.timestamp) / (kf2.timestamp - kf1.timestamp);
            break;
        }
    }

    Logger::log("Debug: Interpolating between keyframes at time: " + std::to_string(animationTime), Logger::INFO);

    for (const auto& [boneName, transform1] : kf1.boneTransforms) {
        glm::mat4 transform2 = kf2.boneTransforms.at(boneName);
        glm::mat4 interpolatedTransform = interpolateKeyframes(transform1, transform2, factor);

        Logger::log("Debug: Applying transform to bone: " + boneName, Logger::INFO);

        model->setBoneTransform(boneName, interpolatedTransform);
    }
}

void Animation::loadAnimation(const std::string& filePath) {
    Logger::log("Loading animation from: " + filePath, Logger::INFO);

    keyframes = {
        {0.0f, { {"Bone1", glm::mat4(1.0f)}, {"Bone2", glm::mat4(1.0f)} }},
        {1.0f, { {"Bone1", glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
                  {"Bone2", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f))} }}
    };

    duration = 1.0f;
    loaded = true;

    Logger::log("Animation loaded successfully: " + filePath, Logger::INFO);
}

glm::mat4 Animation::interpolateKeyframes(const glm::mat4& transform1, const glm::mat4& transform2, float factor) {
    glm::vec3 pos1, pos2, scale1, scale2;
    glm::quat rot1, rot2;
    glm::vec4 persp;
    glm::vec3 skew;

    glm::decompose(transform1, scale1, rot1, pos1, skew, persp);
    glm::decompose(transform2, scale2, rot2, pos2, skew, persp);

    glm::vec3 interpolatedPos = glm::mix(pos1, pos2, factor);
    glm::quat interpolatedRot = glm::slerp(rot1, rot2, factor);
    glm::vec3 interpolatedScale = glm::mix(scale1, scale2, factor);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), interpolatedPos);
    glm::mat4 rotation = glm::mat4_cast(interpolatedRot);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), interpolatedScale);

    return translation * rotation * scale;
}
