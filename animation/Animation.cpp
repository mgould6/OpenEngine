#define GLM_ENABLE_EXPERIMENTAL

#include "Animation.h"
#include "../model/Model.h"
#include "../common_utils/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

Animation::Animation(const std::string& filePath,
    const Model* model)
    : name(filePath)
{
    loadAnimation(filePath, model);
}

/* -------------------------------------------------------------- */
/*  Public: apply final pose to model (simple hold-last approach) */
/* -------------------------------------------------------------- */
void Animation::apply(float animationTimeSeconds, Model* model) const
{
    if (!loaded || keyframes.empty() || model == nullptr)
    {
        Logger::log("Animation::apply called with invalid state",
            Logger::ERROR);
        return;
    }

    std::map<std::string, glm::mat4> pose;
    interpolateKeyframes(animationTimeSeconds, pose);

    for (const auto& pair : pose)
        model->setBoneTransform(pair.first, pair.second);
}

/* -------------------------------------------------------------- */
/*  Public: produce blended pose at time                          */
/* -------------------------------------------------------------- */
void Animation::interpolateKeyframes(float animationTimeSeconds,
    std::map<std::string, glm::mat4>&
    outPose) const
{
    if (keyframes.empty())
        return;

    auto indices = findKeyframeIndices(animationTimeSeconds);
    const Keyframe& kfA = keyframes[indices.first];
    const Keyframe& kfB = keyframes[indices.second];

    float span = kfB.time - kfA.time;
    if (span < 0.0f)
        span += clipDurationSecs;

    float factor = (span > 0.0f)
        ? (animationTimeSeconds - kfA.time) / span
        : 0.0f;
    if (factor < 0.0f)
        factor += 1.0f;

    for (const auto& pairA : kfA.boneTransforms)
    {
        const std::string& bone = pairA.first;
        const glm::mat4& matA = pairA.second;

        auto itB = kfB.boneTransforms.find(bone);
        if (itB != kfB.boneTransforms.end())
            outPose[bone] = interpolateMatrices(matA, itB->second, factor);
        else
            outPose[bone] = matA;
    }
}

/* -------------------------------------------------------------- */
/*  Helper: find surrounding keyframes (seconds domain)           */
/* -------------------------------------------------------------- */
std::pair<size_t, size_t>
Animation::findKeyframeIndices(float timeSeconds) const
{
    const size_t count = keyframes.size();
    if (count == 0)
        return { 0, 0 };

    float t = std::fmod(timeSeconds, clipDurationSecs);
    if (t < 0.0f)
        t += clipDurationSecs;

    for (size_t i = 0; i < count; ++i)
    {
        size_t next = (i + 1) % count;
        if (t >= keyframes[i].time && t < keyframes[next].time)
            return { i, next };
    }
    return { count - 1, 0 };
}

/* -------------------------------------------------------------- */
/*  Helper: blend two 4x4 local matrices (SRT decompose)          */
/* -------------------------------------------------------------- */
glm::mat4 Animation::interpolateMatrices(const glm::mat4& a,
    const glm::mat4& b,
    float            factor) const
{
    glm::vec3   scaleA, translationA, skewA;
    glm::quat   rotA;
    glm::vec4   perspectiveA;
    glm::decompose(a, scaleA, rotA, translationA, skewA, perspectiveA);

    glm::vec3   scaleB, translationB, skewB;
    glm::quat   rotB;
    glm::vec4   perspectiveB;
    glm::decompose(b, scaleB, rotB, translationB, skewB, perspectiveB);

    glm::vec3   scale = glm::mix(scaleA, scaleB, factor);
    glm::quat   rotation = glm::slerp(rotA, rotB, factor);
    glm::vec3   position = glm::mix(translationA, translationB, factor);

    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m *= glm::mat4_cast(glm::normalize(rotation));
    m = glm::scale(m, scale);

    return m;
}

/* -------------------------------------------------------------- */
/*  Load animation from file into seconds-based keyframes         */
/* -------------------------------------------------------------- */
void Animation::loadAnimation(const std::string& filePath,
    const Model* model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || !scene->HasAnimations() || scene->mAnimations[0] == nullptr)
    {
        Logger::log("Assimp failed to load animation: " + filePath,
            Logger::ERROR);
        return;
    }

    const aiAnimation* anim = scene->mAnimations[0];

    durationTicks = static_cast<float>(anim->mDuration);
    ticksPerSecond = (anim->mTicksPerSecond > 0.0f)
        ? static_cast<float>(anim->mTicksPerSecond)
        : 24.0f;
    clipDurationSecs = durationTicks / ticksPerSecond;

    /* convert every key to seconds and store */
    keyframes.clear();

    std::unordered_map<float,
        std::map<std::string, glm::mat4>> tempTimeline;

    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
    {
        const aiNodeAnim* channel = anim->mChannels[c];
        std::string boneName = channel->mNodeName.C_Str();

        const unsigned int maxKeys = std::max({
            channel->mNumPositionKeys,
            channel->mNumRotationKeys,
            channel->mNumScalingKeys });

        for (unsigned int k = 0; k < maxKeys; ++k)
        {
            unsigned int pIdx = std::min(k,
                channel->mNumPositionKeys ? channel->mNumPositionKeys - 1 : 0);
            unsigned int rIdx = std::min(k,
                channel->mNumRotationKeys ? channel->mNumRotationKeys - 1 : 0);
            unsigned int sIdx = std::min(k,
                channel->mNumScalingKeys ? channel->mNumScalingKeys - 1 : 0);

            float timestampTick = 0.0f;
            if (k < channel->mNumRotationKeys)
                timestampTick = static_cast<float>(channel->mRotationKeys[rIdx].mTime);
            else if (k < channel->mNumPositionKeys)
                timestampTick = static_cast<float>(channel->mPositionKeys[pIdx].mTime);
            else
                timestampTick = static_cast<float>(channel->mScalingKeys[sIdx].mTime);

            float timeSecs = timestampTick / ticksPerSecond;

            /* build SRT */
            const aiVector3D& pos = channel->mPositionKeys[pIdx].mValue;
            glm::vec3 position(pos.x, pos.y, pos.z);

            const aiQuaternion& rot = channel->mRotationKeys[rIdx].mValue;
            glm::quat rotation(rot.w, rot.x, rot.y, rot.z);

            const aiVector3D& scl = channel->mScalingKeys[sIdx].mValue;
            glm::vec3 scaleVec(scl.x, scl.y, scl.z);

            glm::mat4 local(1.0f);
            local = glm::translate(local, position);
            local *= glm::mat4_cast(glm::normalize(rotation));
            local = glm::scale(local, scaleVec);

            tempTimeline[timeSecs][boneName] = local;
        }
    }

    /* transfer ordered map into vector */
    for (const auto& t : tempTimeline)
        keyframes.push_back({ t.first, t.second });

    loaded = true;
}

/* -------------------------------------------------------------- */
