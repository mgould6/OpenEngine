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
    : filePath(filePath), loaded(false), duration(0.0f), ticksPerSecond(60.0f) {
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


void Animation::loadAnimation(const std::string& filePath, const Model* model)
{
    Logger::log("Loading animation from: " + filePath, Logger::INFO);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    if (!scene)
    {
        Logger::log("ERROR: Assimp failed!  " +
            std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }
    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        Logger::log("ERROR: Incomplete scene data.", Logger::ERROR);
        return;
    }
    if (!scene->HasAnimations())
    {
        Logger::log("ERROR: No animations in file!", Logger::ERROR);
        return;
    }

    /* ------------------------------------------------------------ */
    aiAnimation* anim = scene->mAnimations[0];

    duration = static_cast<float>(anim->mDuration);
    ticksPerSecond = (anim->mTicksPerSecond > 0.0f)
        ? static_cast<float>(anim->mTicksPerSecond)
        : 24.0f;                 // FBX default fallback

    const float clipSeconds = duration / ticksPerSecond;

    Logger::log("META  tps=" + std::to_string(ticksPerSecond) +
        "  durationTicks=" + std::to_string(duration) +
        "  clipSeconds=" + std::to_string(clipSeconds),
        Logger::INFO);

    /* ------------- build keyframes (logic unchanged) ------------- */
    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        aiNodeAnim* channel = anim->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        // remap non-DEF bones to DEF-* if present in the model
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

        const unsigned int numKeys = std::min(
            channel->mNumPositionKeys,
            channel->mNumRotationKeys
        );

        for (unsigned int j = 0; j < numKeys; ++j)
        {
            float timestamp = static_cast<float>(channel->mPositionKeys[j].mTime);

            // local transform (pos + rot)
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

            glm::mat4 animationLocal =
                glm::translate(glm::mat4(1.0f), position)
                * glm::mat4_cast(glm::normalize(rotation));

            // inject bind pose if truly identity (skip root)
            const bool isAlmostIdentity =
                glm::all(glm::epsilonEqual(animationLocal[0],
                    glm::vec4(1, 0, 0, 0), 0.01f))
                && glm::all(glm::epsilonEqual(animationLocal[1],
                    glm::vec4(0, 1, 0, 0), 0.01f))
                && glm::all(glm::epsilonEqual(animationLocal[2],
                    glm::vec4(0, 0, 1, 0), 0.01f));

            if (isAlmostIdentity && boneName != "root")
            {
                Logger::log("Injecting bind pose for: " + boneName,
                    Logger::WARNING);
                animationLocal = model->getLocalBindPose(boneName);
            }

            timestampToBoneMap[timestamp][boneName] = animationLocal;
        }
    }

    // collapse timestamp map into ordered vector
    for (const auto& kv : timestampToBoneMap)
        keyframes.push_back({ kv.first, kv.second });

    loaded = true;
    Logger::log("Animation loaded.  keyframes=" +
        std::to_string(keyframes.size()), Logger::INFO);
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
