#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_set>

Animation::Animation(const std::string& filePath, const Model* model)
    : filePath(filePath), loaded(false), duration(0.0f), ticksPerSecond(30.0f) {
    loadAnimation(filePath, model);
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


void Animation::interpolateKeyframes(float animationTime, std::map<std::string, glm::mat4>& finalBoneMatrices) const {
    if (keyframes.empty()) {
        Logger::log("ERROR: No keyframes available for interpolation!", Logger::ERROR);
        return;
    }

    Logger::log("INFO: Interpolating keyframes at animation time: " + std::to_string(animationTime), Logger::INFO);
    Logger::log("INFO: Keyframe count: " + std::to_string(keyframes.size()), Logger::INFO);

    // Find the two keyframes surrounding the current time
    const Keyframe* prev = nullptr;
    const Keyframe* next = nullptr;

    for (size_t i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].timestamp > animationTime) {
            next = &keyframes[i];
            if (i > 0)
                prev = &keyframes[i - 1];
            break;
        }
    }

    if (!prev) prev = &keyframes.front();
    if (!next) next = &keyframes.back();

    float dt = next->timestamp - prev->timestamp;
    float factor = (dt > 0.0f) ? (animationTime - prev->timestamp) / dt : 0.0f;

    for (const auto& [boneName, transform1] : prev->boneTransforms) {
        if (next->boneTransforms.find(boneName) != next->boneTransforms.end()) {
            const glm::mat4& transform2 = next->boneTransforms.at(boneName);
            glm::mat4 interpolated = interpolateKeyframes(transform1, transform2, factor);
            finalBoneMatrices[boneName] = interpolated;

            Logger::log("INFO: Interpolated bone: " + boneName + " with factor: " + std::to_string(factor), Logger::INFO);

            if (boneName == "DEF-upper_arm.L" || boneName == "DEF-forearm.L") {
                Logger::log("ANIM FINAL MATRIX: " + boneName + " @ time " + std::to_string(animationTime), Logger::WARNING);
                Logger::log(glm::to_string(interpolated), Logger::WARNING);
            }
        }
        else {
            finalBoneMatrices[boneName] = transform1;
            Logger::log("INFO: Bone " + boneName + " has no match in next keyframe — using fallback.", Logger::DEBUG);
        }
    }
}

void Animation::loadAnimation(const std::string& filePath, const Model* model) {
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

    aiAnimation* anim = scene->mAnimations[0];
    duration = anim->mDuration;
    ticksPerSecond = anim->mTicksPerSecond > 0.0 ? anim->mTicksPerSecond : 30.0f;

    for (unsigned int i = 0; i < anim->mNumChannels; i++) {
        aiNodeAnim* channel = anim->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        if (boneName.find("DEF-") != 0) {
            std::string tryDEF = "DEF-" + boneName;
            if (model->hasBone(tryDEF)) {
                Logger::log("Remapping animation bone '" + boneName + "' to '" + tryDEF + "'", Logger::INFO);
                boneName = tryDEF;
            }
        }

        bonesWithKeyframes.insert(boneName);
        animatedBones.push_back(boneName);

        unsigned int numKeys = std::min(channel->mNumPositionKeys, channel->mNumRotationKeys);

        for (unsigned int j = 0; j < numKeys; j++) {
            float timestamp = static_cast<float>(channel->mPositionKeys[j].mTime);

            glm::vec3 position(
                channel->mPositionKeys[j].mValue.x,
                channel->mPositionKeys[j].mValue.y,
                channel->mPositionKeys[j].mValue.z
            );

            glm::quat rotation(
                channel->mRotationKeys[j].mValue.w,
                channel->mRotationKeys[j].mValue.x,
                channel->mRotationKeys[j].mValue.y,
                channel->mRotationKeys[j].mValue.z
            );

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 rotationMat = glm::mat4_cast(glm::normalize(rotation));
            glm::mat4 animationLocalTransform = translation * rotationMat;

            // Normalize animation to be relative to bind pose
            glm::mat4 bindPose = model->getBindPoseGlobalTransform(boneName);
            glm::mat4 correctedLocal = glm::inverse(bindPose) * animationLocalTransform;

            // FORCE clean identity at T=0 for DEF bones
            if (timestamp == 0.0f && boneName.find("DEF-") == 0) {
                correctedLocal = glm::mat4(1.0f);
            }

            if (timestamp == 0.0f && (boneName == "DEF-upper_arm.L" || boneName == "DEF-forearm.L")) {
                Logger::log("=== DEBUG: Animation Transform at T=0 for " + boneName + " ===", Logger::WARNING);
                Logger::log("animationLocalTransform:\n" + glm::to_string(animationLocalTransform), Logger::WARNING);
                Logger::log("bindPose:\n" + glm::to_string(bindPose), Logger::WARNING);
                Logger::log("correctedLocal (post-fix):\n" + glm::to_string(correctedLocal), Logger::WARNING);
            }

            timestampToBoneMap[timestamp][boneName] = correctedLocal;
        }
    }

    for (const auto& [timestamp, boneMap] : timestampToBoneMap) {
        keyframes.push_back({ timestamp, boneMap });
    }

    loaded = true;
    Logger::log("Animation loaded and re-normalized relative to model bind pose.", Logger::INFO);
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
