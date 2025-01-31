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
    if (!loaded || keyframes.empty()) {
        Logger::log("Error: No keyframes loaded in animation!", Logger::ERROR);
        return;
    }

    Keyframe kf1, kf2;
    float factor = 0.0f;
    bool keyframeFound = false;

    // Find the two keyframes surrounding the current animation time
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (animationTime >= keyframes[i].timestamp && animationTime <= keyframes[i + 1].timestamp) {
            kf1 = keyframes[i];
            kf2 = keyframes[i + 1];
            factor = (animationTime - kf1.timestamp) / (kf2.timestamp - kf1.timestamp);
            keyframeFound = true;
            break;
        }
    }

    if (!keyframeFound) {
        Logger::log("Error: No valid keyframe pair found for animation time: " + std::to_string(animationTime), Logger::ERROR);
        return;
    }

    Logger::log("Debug: Interpolating keyframes at time: " + std::to_string(animationTime), Logger::INFO);

    std::unordered_map<std::string, glm::mat4> globalBoneTransforms;

    for (const auto& [boneName, transform1] : kf1.boneTransforms) {
        if (kf2.boneTransforms.find(boneName) == kf2.boneTransforms.end()) {
            Logger::log("Warning: Bone " + boneName + " not found in second keyframe!", Logger::WARNING);
            continue;
        }

        glm::mat4 transform2 = kf2.boneTransforms.at(boneName);
        glm::mat4 interpolatedTransform = interpolateKeyframes(transform1, transform2, factor);

        // Debugging: Print keyframe transformation values
        if (boneName == "root" || boneName == "DEF-spine" || boneName == "DEF-thigh.L") {
            Logger::log("Debug: Bone " + boneName + " Transform from Keyframe 1: " +
                std::to_string(transform1[3][0]) + ", " +
                std::to_string(transform1[3][1]) + ", " +
                std::to_string(transform1[3][2]), Logger::INFO);

            Logger::log("Debug: Bone " + boneName + " Transform from Keyframe 2: " +
                std::to_string(transform2[3][0]) + ", " +
                std::to_string(transform2[3][1]) + ", " +
                std::to_string(transform2[3][2]), Logger::INFO);

            Logger::log("Debug: Bone " + boneName + " Interpolated Transform: " +
                std::to_string(interpolatedTransform[3][0]) + ", " +
                std::to_string(interpolatedTransform[3][1]) + ", " +
                std::to_string(interpolatedTransform[3][2]), Logger::INFO);
        }

        globalBoneTransforms[boneName] = interpolatedTransform;
    }

    // Debugging: Check if keyframe data is actually being modified
    if (globalBoneTransforms.empty()) {
        Logger::log("Error: No valid bone transforms computed!", Logger::ERROR);
        return;
    }

    // Apply hierarchy: Ensure child bones inherit parent transformations
    for (const auto& [boneName, transform] : globalBoneTransforms) {
        std::string parentName = model->getBoneParent(boneName);
        if (!parentName.empty() && globalBoneTransforms.find(parentName) != globalBoneTransforms.end()) {
            globalBoneTransforms[boneName] = globalBoneTransforms[parentName] * transform;
        }

        model->setBoneTransform(boneName, globalBoneTransforms[boneName]);

        if (boneName == "root" || boneName == "DEF-spine" || boneName == "DEF-thigh.L") {
            Logger::log("Debug: Bone " + boneName + " Final Transform: " +
                std::to_string(globalBoneTransforms[boneName][3][0]) + ", " +
                std::to_string(globalBoneTransforms[boneName][3][1]) + ", " +
                std::to_string(globalBoneTransforms[boneName][3][2]), Logger::INFO);
        }
    }

    // Force Root Bone Movement for Debugging
    if (globalBoneTransforms.find("root") != globalBoneTransforms.end()) {
        globalBoneTransforms["root"][3][0] += sin(animationTime) * 1.5f;
        globalBoneTransforms["root"][3][1] += cos(animationTime) * 1.5f;
        globalBoneTransforms["root"][3][2] += sin(animationTime) * 1.5f;
        Logger::log("Debug: Forced Root Bone Movement: " +
            std::to_string(globalBoneTransforms["root"][3][0]) + ", " +
            std::to_string(globalBoneTransforms["root"][3][1]) + ", " +
            std::to_string(globalBoneTransforms["root"][3][2]), Logger::INFO);
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
