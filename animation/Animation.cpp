#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation::Animation(const std::string& filePath)
    : filePath(filePath), loaded(false), duration(0.0f), ticksPerSecond(30.0f) { // Default TPS to 30.0
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
        Logger::log("ERROR: Animation is not loaded or has no keyframes!", Logger::ERROR);
        return;
    }

    Logger::log("DEBUG: Entering apply() at time: " + std::to_string(animationTime), Logger::INFO);

    std::unordered_map<std::string, glm::mat4> globalBoneTransforms;

    for (const auto& keyframe : keyframes) {
        if (animationTime >= keyframe.timestamp) {
            for (const auto& [boneName, transform] : keyframe.boneTransforms) {
                globalBoneTransforms[boneName] = transform;
            }
        }
    }

    if (globalBoneTransforms.empty()) {
        Logger::log("ERROR: No valid bone transforms computed!", Logger::ERROR);
        return;
    }

    for (const auto& [boneName, transform] : globalBoneTransforms) {
        model->setBoneTransform(boneName, transform);
        Logger::log("DEBUG: Applied Transform to Bone " + boneName, Logger::INFO);
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
    aiAnimation* anim = scene->mAnimations[0];

    duration = anim->mDuration;
    Logger::log("DEBUG: Animation Duration: " + std::to_string(duration), Logger::INFO);

    if (anim->mTicksPerSecond > 0.0) {
        ticksPerSecond = anim->mTicksPerSecond;
    }
    else {
        Logger::log("WARNING: Animation has no ticks per second set. Defaulting to 30.0", Logger::WARNING);
        ticksPerSecond = 30.0f;
    }

    for (unsigned int i = 0; i < anim->mNumChannels; i++) {
        aiNodeAnim* channel = anim->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        if (channel->mNumPositionKeys == 0 && channel->mNumRotationKeys == 0) {
            Logger::log("WARNING: Bone " + boneName + " has no valid keyframes!", Logger::WARNING);
            continue;
        }

        for (unsigned int j = 0; j < channel->mNumPositionKeys; j++) {
            float timestamp = static_cast<float>(channel->mPositionKeys[j].mTime);
            glm::vec3 position(channel->mPositionKeys[j].mValue.x,
                channel->mPositionKeys[j].mValue.y,
                channel->mPositionKeys[j].mValue.z);

            glm::quat rotation(channel->mRotationKeys[j].mValue.w,
                channel->mRotationKeys[j].mValue.x,
                channel->mRotationKeys[j].mValue.y,
                channel->mRotationKeys[j].mValue.z);

            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
            keyframes.push_back({ timestamp, { { boneName, transform } } });
        }
    }

    loaded = true;
}

void Animation::interpolateKeyframes(float animationTime, std::map<std::string, glm::mat4>& finalBoneMatrices) const {
    for (const auto& keyframe : keyframes) {
        if (animationTime >= keyframe.timestamp) {
            for (const auto& [boneName, transform] : keyframe.boneTransforms) {
                finalBoneMatrices[boneName] = transform;
            }
        }
    }
}

glm::mat4 Animation::interpolateKeyframes(const glm::mat4& transform1, const glm::mat4& transform2, float factor) const {
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

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), interpolatedPos);
    glm::mat4 rotation = glm::mat4_cast(interpolatedRot);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), interpolatedScale);

    return translation * rotation * scale;
}
