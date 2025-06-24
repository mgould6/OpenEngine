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
#include <algorithm>          // std::max

Animation::Animation(const std::string& filePath, const Model* model)
    : loaded(false),
    duration(0.0f),
    ticksPerSecond(60.0f),
    name(filePath)            // <-- save the clip name
{
    loadAnimation(filePath, model);
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
            glm::mat4 interpolated = interpolateMatrices(transform1, transform2, factor);
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


void Animation::loadAnimation(const std::string& filePath,
    const Model* model)
{
    Logger::log("Loading animation from: " + filePath, Logger::INFO);

    /* ---------- Assimp read ---------- */
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        Logger::log("ERROR: Assimp failed or scene incomplete!  "
            + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }
    if (!scene->HasAnimations())
    {
        Logger::log("ERROR: No animations in file!", Logger::ERROR);
        return;
    }

    /* ---------- meta ---------- */
    aiAnimation* anim = scene->mAnimations[0];

    duration = static_cast<float>(anim->mDuration);
    ticksPerSecond = (anim->mTicksPerSecond > 0.0f)
        ? static_cast<float>(anim->mTicksPerSecond)
        : 24.0f;            // FBX fallback

    Logger::log("META  tps=" + std::to_string(ticksPerSecond) +
        "  durationTicks=" + std::to_string(duration) +
        "  clipSeconds=" + std::to_string(duration / ticksPerSecond),
        Logger::INFO);

    /* ---------- harvest keyframes ---------- */
    timestampToBoneMap.clear();
    bonesWithKeyframes.clear();
    animatedBones.clear();

    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        aiNodeAnim* channel = anim->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        /* map non-DEF names to model bones if possible */
        if (boneName.rfind("DEF-", 0) != 0)
        {
            std::string tryDEF = "DEF-" + boneName;
            if (model->hasBone(tryDEF))
            {
                Logger::log("Remapping '" + boneName +
                    "' -> '" + tryDEF + "'", Logger::INFO);
                boneName = tryDEF;
            }
        }

        bonesWithKeyframes.insert(boneName);
        animatedBones.push_back(boneName);

        const unsigned int maxKeys = std::max({
            channel->mNumPositionKeys,
            channel->mNumRotationKeys,
            channel->mNumScalingKeys
            });

        for (unsigned int k = 0; k < maxKeys; ++k)
        {
            /* clamp indices so we reuse the final key once we run out */
            unsigned int pIdx = std::min(k,
                channel->mNumPositionKeys ? channel->mNumPositionKeys - 1 : 0);
            unsigned int rIdx = std::min(k,
                channel->mNumRotationKeys ? channel->mNumRotationKeys - 1 : 0);
            unsigned int sIdx = std::min(k,
                channel->mNumScalingKeys ? channel->mNumScalingKeys - 1 : 0);

            /* choose timestamp from whatever track we still have */
            float timestamp = 0.0f;
            if (channel->mNumRotationKeys && k < channel->mNumRotationKeys)
                timestamp = static_cast<float>(channel->mRotationKeys[rIdx].mTime);
            else if (channel->mNumPositionKeys && k < channel->mNumPositionKeys)
                timestamp = static_cast<float>(channel->mPositionKeys[pIdx].mTime);
            else
                timestamp = static_cast<float>(channel->mScalingKeys[sIdx].mTime);

            /* fetch transforms */
            const aiVector3D& pos = channel->mPositionKeys[pIdx].mValue;
            glm::vec3 position(pos.x, pos.y, pos.z);

            const aiQuaternion& rot = channel->mRotationKeys[rIdx].mValue;
            glm::quat rotation(rot.w, rot.x, rot.y, rot.z);

            const aiVector3D& scl = channel->mScalingKeys[sIdx].mValue;
            glm::vec3 scaleVec(scl.x, scl.y, scl.z);

            glm::mat4 local =
                glm::translate(glm::mat4(1.0f), position) *
                glm::mat4_cast(glm::normalize(rotation)) *
                glm::scale(glm::mat4(1.0f), scaleVec);

            timestampToBoneMap[timestamp][boneName] = local;
        }
    }

    /* ---------- push map into vector ---------- */
    keyframes.clear();
    for (const auto& kv : timestampToBoneMap)
        keyframes.push_back({ kv.first, kv.second });

    /* 60-fps resample block REMOVED – we now keep the original keys */

    loaded = true;
    Logger::log("Animation loaded successfully.", Logger::INFO);
}

glm::mat4 Animation::interpolateMatrices(const glm::mat4& transform1, const glm::mat4& transform2, float factor) const {
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
