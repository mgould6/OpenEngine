#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

    Logger::log("Debug: Applying animation at time: " + std::to_string(animationTime), Logger::INFO);

    Keyframe kf1, kf2;
    float factor = 0.0f;
    bool keyframeFound = false;

    // Locate the correct keyframe range for interpolation
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

    std::unordered_map<std::string, glm::mat4> globalBoneTransforms;

    // Interpolate keyframes for each bone
    for (const auto& [boneName, transform1] : kf1.boneTransforms) {
        if (kf2.boneTransforms.find(boneName) == kf2.boneTransforms.end()) {
            Logger::log("Warning: Bone " + boneName + " missing in second keyframe!", Logger::WARNING);
            continue;
        }

        glm::mat4 transform2 = kf2.boneTransforms.at(boneName);
        glm::mat4 interpolatedTransform = interpolateKeyframes(transform1, transform2, factor);

        Logger::log("Debug: Bone " + boneName + " - Interpolated Transform", Logger::INFO);
        for (int row = 0; row < 4; row++) {
            Logger::log(
                std::to_string(interpolatedTransform[row][0]) + " " +
                std::to_string(interpolatedTransform[row][1]) + " " +
                std::to_string(interpolatedTransform[row][2]) + " " +
                std::to_string(interpolatedTransform[row][3]), Logger::INFO);
        }

        globalBoneTransforms[boneName] = interpolatedTransform;
    }

    if (globalBoneTransforms.empty()) {
        Logger::log("Error: No valid bone transforms computed!", Logger::ERROR);
        return;
    }

    // Apply transforms to model bones
    for (const auto& [boneName, transform] : globalBoneTransforms) {
        std::string parentName = model->getBoneParent(boneName);
        if (!parentName.empty() && globalBoneTransforms.find(parentName) != globalBoneTransforms.end()) {
            globalBoneTransforms[boneName] = globalBoneTransforms[parentName] * transform;
        }

        model->setBoneTransform(boneName, globalBoneTransforms[boneName]);

        Logger::log("Debug: Applied Transform to Bone " + boneName + " | Pos: " +
            std::to_string(globalBoneTransforms[boneName][3][0]) + ", " +
            std::to_string(globalBoneTransforms[boneName][3][1]) + ", " +
            std::to_string(globalBoneTransforms[boneName][3][2]), Logger::INFO);
    }
}

void Animation::loadAnimation(const std::string& filePath) {
    Logger::log("Loading animation from: " + filePath, Logger::INFO);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene) {
        Logger::log("ERROR: Assimp failed to load file! Error: " + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::log("ERROR: Incomplete scene data in animation file.", Logger::ERROR);
        return;
    }

    if (!scene->HasAnimations()) {
        Logger::log("ERROR: No animations found in file!", Logger::ERROR);
        return;
    }

    Logger::log("INFO: Successfully found animations in file.", Logger::INFO);
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

    Logger::log("Interpolating Keyframes", Logger::INFO);
    Logger::log("Bone Transform Interpolation at factor: " + std::to_string(factor), Logger::INFO);
    Logger::log("Start Position: " + std::to_string(pos1.x) + ", " + std::to_string(pos1.y) + ", " + std::to_string(pos1.z), Logger::INFO);
    Logger::log("End Position: " + std::to_string(pos2.x) + ", " + std::to_string(pos2.y) + ", " + std::to_string(pos2.z), Logger::INFO);
    Logger::log("Interpolated Position: " + std::to_string(interpolatedPos.x) + ", " + std::to_string(interpolatedPos.y) + ", " + std::to_string(interpolatedPos.z), Logger::INFO);
    Logger::log("Interpolated Rotation: " + std::to_string(interpolatedRot.x) + ", " + std::to_string(interpolatedRot.y) + ", " + std::to_string(interpolatedRot.z) + ", " + std::to_string(interpolatedRot.w), Logger::INFO);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), interpolatedPos);
    glm::mat4 rotation = glm::mat4_cast(interpolatedRot);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), interpolatedScale);

    return translation * rotation * scale;
}
