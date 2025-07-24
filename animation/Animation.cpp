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
#include <glm/gtx/string_cast.hpp>

Animation::Animation(const std::string& filePath,
    const Model* model)
    : name(filePath), modelRef(model)
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
    // 1. Decompose both matrices into TRS components
    glm::vec3 posA, posB, scaleA, scaleB;
    glm::quat rotA, rotB;

    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(a, scaleA, rotA, posA, skew, perspective);
    glm::decompose(b, scaleB, rotB, posB, skew, perspective);  // reuse vars

    // Ensure shortest rotation path
    if (glm::dot(rotA, rotB) < 0.0f)
        rotB = -rotB;

    // 2. Interpolate components
    glm::vec3 posFinal = glm::mix(posA, posB, factor);
    glm::quat rotFinal = glm::slerp(rotA, rotB, factor);
    glm::vec3 scaleFinal = glm::mix(scaleA, scaleB, factor);

    // 3. Recompose final matrix
    glm::mat4 T = glm::translate(glm::mat4(1.0f), posFinal);
    glm::mat4 R = glm::mat4_cast(rotFinal);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFinal);

    return T * R * S;
}




/* -------------------------------------------------------------- */
/*  Blend pose - uses union of bones in kfA and kfB               */
/* -------------------------------------------------------------- */
void Animation::interpolateKeyframes(float animationTime, std::map<std::string, glm::mat4>& outPose) const
{
    if (keyframes.empty())
        return;

    // Handle case with only one frame (no interpolation)
    if (keyframes.size() == 1)
    {
        outPose = keyframes[0].boneTransforms;
        return;
    }

    // Find keyframes to interpolate between
    size_t startFrame = 0;
    size_t endFrame = 0;

    for (size_t i = 0; i < keyframes.size() - 1; ++i)
    {
        if (animationTime < keyframes[i + 1].time)
        {
            startFrame = i;
            endFrame = i + 1;
            break;
        }
    }

    // Edge case: animationTime beyond last frame
    if (endFrame == 0)
    {
        startFrame = keyframes.size() - 2;
        endFrame = keyframes.size() - 1;
    }

    const Keyframe& kf0 = keyframes[startFrame];
    const Keyframe& kf1 = keyframes[endFrame];

    float t0 = kf0.time;
    float t1 = kf1.time;
    float lerpFactor = (animationTime - t0) / (t1 - t0);

    // Debug output when near frame 28
    if (animationTime >= 0.0f)
    {
        Logger::log("DEBUG: animationTime = " + std::to_string(animationTime), Logger::WARNING);
        Logger::log("DEBUG: Interpolating between keyframes " + std::to_string(startFrame) +
            " (time = " + std::to_string(t0) + ") and " +
            std::to_string(endFrame) + " (time = " + std::to_string(t1) + ")",
            Logger::WARNING);
        Logger::log("DEBUG: Lerp factor = " + std::to_string(lerpFactor), Logger::WARNING);
    }

    for (const auto& [boneName, mat0] : kf0.boneTransforms)
    {
        glm::mat4 mat1 = kf1.boneTransforms.count(boneName) ?
            kf1.boneTransforms.at(boneName) :
            mat0;

        glm::mat4 interp = glm::mat4(0.0f);

        for (int col = 0; col < 4; ++col)
        {
            interp[col] = glm::mix(mat0[col], mat1[col], lerpFactor);
        }        outPose[boneName] = interp;
    }
}



/* -------------------------------------------------------------- */



//helper for load animation
/* utility – compare two mat4s with an epsilon --------------------------------*/
static bool matNearlyEqual(const glm::mat4& a,
    const glm::mat4& b,
    float            eps = 1e-4f)
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (std::fabs(a[c][r] - b[c][r]) > eps)
                return false;
    return true;
}

/* -------------------------------------------------------------- */
/*  Animation::loadAnimation                                      */
/*  – parses the FBX clip, auto-detects fps if mTicksPerSecond=0  */
/*  – forward + reverse fills so every keyframe has every bone    */
/*  – strips any bind-pose frame and re-bases the timeline        */
/* -------------------------------------------------------------- */
void Animation::loadAnimation(const std::string& filePath,
    const Model* model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || !scene->HasAnimations() || !scene->mAnimations[0])
    {
        Logger::log("Assimp failed to load animation: " + filePath,
            Logger::ERROR);
        loaded = false;
        return;
    }

    const aiAnimation* src = scene->mAnimations[0];
    durationTicks = static_cast<float>(src->mDuration);

    /* ----------- choose ticksPerSecond ------------------------ */
    if (src->mTicksPerSecond > 0.0)
    {
        ticksPerSecond = static_cast<float>(src->mTicksPerSecond);
    }
    else
    {
        /* detect fps from key spacing                            */
        float minDelta = std::numeric_limits<float>::max();
        for (unsigned int c = 0; c < src->mNumChannels; ++c)
        {
            const aiNodeAnim* ch = src->mChannels[c];
            for (unsigned int k = 1; k < ch->mNumRotationKeys; ++k)
            {
                float d = float(ch->mRotationKeys[k].mTime
                    - ch->mRotationKeys[k - 1].mTime);
                if (d > 0.0f && d < minDelta) minDelta = d;
            }
        }

        if (std::fabs(minDelta - 1.0f) < 1e-3f)
        {
            if (std::fmod(durationTicks, 60.0f) < 1e-3f)
                ticksPerSecond = 60.0f;
            else if (std::fmod(durationTicks, 30.0f) < 1e-3f)
                ticksPerSecond = 30.0f;
            else
                ticksPerSecond = 24.0f;
        }
        else
        {
            ticksPerSecond = 1.0f / std::max(minDelta, 1e-6f);
        }
    }
    clipDurationSecs = durationTicks / ticksPerSecond;

    /* ----------- gather sparse timeline ----------------------- */
    std::unordered_map<float,
        std::map<std::string, glm::mat4>> sparse;

    for (unsigned int c = 0; c < src->mNumChannels; ++c)
    {
        const aiNodeAnim* ch = src->mChannels[c];
        std::string bone = ch->mNodeName.C_Str();

        unsigned int maxK = std::max({ ch->mNumPositionKeys,
                                       ch->mNumRotationKeys,
                                       ch->mNumScalingKeys });

        for (unsigned int k = 0; k < maxK; ++k)
        {
            unsigned p = std::min(k, ch->mNumPositionKeys ? ch->mNumPositionKeys - 1 : 0u);
            unsigned r = std::min(k, ch->mNumRotationKeys ? ch->mNumRotationKeys - 1 : 0u);
            unsigned s = std::min(k, ch->mNumScalingKeys ? ch->mNumScalingKeys - 1 : 0u);

            float tick = (k < ch->mNumRotationKeys) ? float(ch->mRotationKeys[r].mTime)
                : (k < ch->mNumPositionKeys) ? float(ch->mPositionKeys[p].mTime)
                : float(ch->mScalingKeys[s].mTime);

            float tSec = tick / ticksPerSecond;

            const aiVector3D& pos = ch->mPositionKeys[p].mValue;
            const aiQuaternion& rot = ch->mRotationKeys[r].mValue;
            const aiVector3D& scl = ch->mScalingKeys[s].mValue;

            glm::mat4 m(1.0f);
            m = glm::translate(m, { pos.x, pos.y, pos.z });
            m *= glm::mat4_cast(glm::normalize(glm::quat(rot.w, rot.x, rot.y, rot.z)));

            glm::vec3 scale(scl.x, scl.y, scl.z);
            if (glm::length(scale - glm::vec3(1.0f)) > 0.01f)
                m = glm::scale(m, scale);

            // Optional: discard scaling altogether for rigging safety
            // m = SkeletonPose::removeScale(m);

            sparse[tSec][bone] = m;
        }
    }

    /* ----------- move to ordered vector ----------------------- */
    keyframes.clear();
    for (const auto& p : sparse)
        keyframes.push_back({ p.first, p.second });

    if (keyframes.empty())
    {
        loaded = false;
        return;
    }

    /* forward-fill --------------------------------------------- */
    std::map<std::string, glm::mat4> last = keyframes.front().boneTransforms;
    for (Keyframe& kf : keyframes)
    {
        for (const auto& kv : last)
            if (!kf.boneTransforms.count(kv.first))
                kf.boneTransforms[kv.first] = kv.second;
        last = kf.boneTransforms;
    }

    /* reverse-fill --------------------------------------------- */
    std::map<std::string, glm::mat4> next = keyframes.back().boneTransforms;
    for (int i = int(keyframes.size()) - 1; i >= 0; --i)
    {
        for (const auto& kv : next)
            if (!keyframes[i].boneTransforms.count(kv.first))
                keyframes[i].boneTransforms[kv.first] = kv.second;
        next = keyframes[i].boneTransforms;
    }

    /* strip bind-pose frame at t == 0 if present                 */
    if (keyframes.size() > 1 && keyframes[0].time < 1e-6f)
        keyframes.erase(keyframes.begin());

    /* re-base timeline to start at 0                             */
    if (!keyframes.empty())
    {
        float t0 = keyframes.front().time;
        clipDurationSecs -= t0;
        for (Keyframe& kf : keyframes)
        {
            kf.time -= t0;
            if (kf.time < 0.0f) kf.time += clipDurationSecs;
        }
        std::sort(keyframes.begin(), keyframes.end(),
            [](const Keyframe& a, const Keyframe& b)
            { return a.time < b.time; });
    }

    /* --------------------------------------------------------------
   REMOVE last key-frame if it holds the same pose as the first
   (common when exporters append the bind pose at the end)
-------------------------------------------------------------- */
    if (keyframes.size() > 1)
    {
        const Keyframe& firstKF = keyframes.front();
        const Keyframe& lastKF = keyframes.back();

        bool samePose = true;
        for (const auto& kv : firstKF.boneTransforms)
        {
            auto it = lastKF.boneTransforms.find(kv.first);
            if (it == lastKF.boneTransforms.end() ||
                !matNearlyEqual(kv.second, it->second))
            {
                samePose = false;
                break;
            }
        }


        if (samePose)
        {
            /* shorten the clip so we never sample the duplicate */
            clipDurationSecs = lastKF.time;
            keyframes.pop_back();
        }

        // Sanitize all keyframes: replace invalid bone matrices with bind pose
        for (Keyframe& kf : keyframes)
        {
            for (auto& [boneName, mat] : kf.boneTransforms)
            {
                bool isZero =
                    glm::length(glm::vec4(mat[0])) < 1e-5f &&
                    glm::length(glm::vec4(mat[1])) < 1e-5f &&
                    glm::length(glm::vec4(mat[2])) < 1e-5f &&
                    glm::length(glm::vec4(mat[3])) < 1e-5f;

                if (isZero)
                {
                    Logger::log("Sanitizing invalid matrix for bone '" + boneName +
                        "' in keyframe at t=" + std::to_string(kf.time) +
                        ". Using bind pose.", Logger::WARNING);
                    if (modelRef)
                    {
                        mat = modelRef->getLocalBindPose(boneName);
                    }
                    else
                    {
                        Logger::log("ERROR: modelRef is null — cannot sanitize bone '" +
                            boneName + "' with bind pose fallback.", Logger::ERROR);
                    }

                }
            }
        }
    }

    loaded = true;
    Logger::log("Loaded clip '" + filePath + "' fps=" +
        std::to_string(ticksPerSecond), Logger::INFO);
}

// Animation.cpp
void Animation::checkBindMismatch(const Model* model)
{
    const float EPS = 1e-4f;;
    bool anyDiff = false;

    for (const auto& bone : model->getBones())
    {
        glm::mat4 bindLocal = model->getLocalBindPose(bone.name);
        glm::mat4 firstLocal = getLocalMatrixAtTime(bone.name, 1e-5f);
        if (!matNearlyEqual(bindLocal, firstLocal, EPS))
        {
            glm::vec3 bindT(bindLocal[3]);     // extract translation
            glm::vec3 poseT(firstLocal[3]);
            glm::vec3 delta = poseT - bindT;
            Logger::log("Delta Translation for " + name + ": " + glm::to_string(delta), Logger::WARNING);

        }
    }
    mismatchChecked = true;          // <- tiny flag in Animation class
}

glm::mat4 Animation::getLocalMatrixAtTime(const std::string& bone,
    float t) const
{
    if (keyframes.empty())
        return glm::mat4(1.0f);

    auto idx = findKeyframeIndices(t);
    const glm::mat4& A = keyframes[idx.first].boneTransforms.at(bone);
    const glm::mat4& B = keyframes[idx.second].boneTransforms.at(bone);

    float span = keyframes[idx.second].time - keyframes[idx.first].time;
    if (span < 0.0f) span += clipDurationSecs;

    float factor = (span > 0.0f) ?
        std::fmod(t - keyframes[idx.first].time + clipDurationSecs,
            clipDurationSecs) / span : 0.0f;

    return interpolateMatrices(A, B, factor);
}