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
/*  - fills missing bones so every keyframe is complete           */
/*  - removes bind pose frame and re-bases timeline               */
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

    /* first pass: collect raw keys ----------------------------- */
    std::unordered_map<float,
        std::map<std::string, glm::mat4>> tempTimeline;
    std::unordered_set<std::string> allBones;

    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
    {
        const aiNodeAnim* channel = anim->mChannels[c];
        std::string boneName = channel->mNodeName.C_Str();
        allBones.insert(boneName);

        unsigned int maxKeys = std::max({
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

            float tick = 0.0f;
            if (k < channel->mNumRotationKeys)
                tick = static_cast<float>(channel->mRotationKeys[rIdx].mTime);
            else if (k < channel->mNumPositionKeys)
                tick = static_cast<float>(channel->mPositionKeys[pIdx].mTime);
            else
                tick = static_cast<float>(channel->mScalingKeys[sIdx].mTime);

            float tSec = tick / ticksPerSecond;

            /* build SRT ----------------------------------------- */
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

            tempTimeline[tSec][boneName] = local;
        }
    }

    /* second pass: move to vector ------------------------------- */
    keyframes.clear();
    for (const auto& pair : tempTimeline)
        keyframes.push_back({ pair.first, pair.second });

    /* forward-fill missing bones so every frame is complete ----- */
    std::map<std::string, glm::mat4> lastPose;

    for (Keyframe& kf : keyframes)
    {
        for (const auto& lp : lastPose)
            if (kf.boneTransforms.find(lp.first) == kf.boneTransforms.end())
                kf.boneTransforms[lp.first] = lp.second;

        for (const auto& cur : kf.boneTransforms)
            lastPose[cur.first] = cur.second;
    }

    /* remove bind-pose frame and re-base timeline --------------- */
    if (keyframes.size() > 1)
    {
        float minPositive = std::numeric_limits<float>::max();
        for (const auto& kf : keyframes)
            if (kf.time > 1e-6f && kf.time < minPositive)
                minPositive = kf.time;

        if (minPositive < std::numeric_limits<float>::max())
        {
            clipDurationSecs -= minPositive;

            for (auto& kf : keyframes)
            {
                kf.time -= minPositive;
                if (kf.time < 0.0f)
                    kf.time += clipDurationSecs;
            }

            std::sort(keyframes.begin(), keyframes.end(),
                [](const Keyframe& a, const Keyframe& b)
                { return a.time < b.time; });

            if (std::fabs(keyframes[0].time) > 1e-6f)
                keyframes.insert(keyframes.begin(),
                    { 0.0f, keyframes.back().boneTransforms });
        }
    }

    loaded = true;
}
/* -------------------------------------------------------------- */


/* -------------------------------------------------------------- */
/*  Blend pose - uses union of bones in kfA and kfB               */
/* -------------------------------------------------------------- */
void Animation::interpolateKeyframes(float animationTimeSeconds,
    std::map<std::string, glm::mat4>& outPose) const
{
    if (keyframes.empty())
        return;

    auto idx = findKeyframeIndices(animationTimeSeconds);
    const Keyframe& kfA = keyframes[idx.first];
    const Keyframe& kfB = keyframes[idx.second];

    float span = kfB.time - kfA.time;
    if (span < 0.0f) span += clipDurationSecs;

    float factor = (span > 0.0f)
        ? std::fmod(animationTimeSeconds - kfA.time + clipDurationSecs,
            clipDurationSecs) / span
        : 0.0f;

    outPose.clear();

    /* default to kfA pose -------------------------------------- */
    for (const auto& p : kfA.boneTransforms)
        outPose[p.first] = p.second;

    /* blend where kfB has data --------------------------------- */
    for (const auto& pB : kfB.boneTransforms)
    {
        const std::string& bone = pB.first;

        const glm::mat4& matA = outPose.count(bone) ? outPose[bone]
            : pB.second;
        outPose[bone] = interpolateMatrices(matA, pB.second, factor);
    }
}
/* -------------------------------------------------------------- */
