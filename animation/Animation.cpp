#define GLM_ENABLE_EXPERIMENTAL

#include "Animation.h"
#include "../model/Model.h"
#include "../common_utils/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

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

    // DEBUG OVERRIDE: Force hold-frame playback at frame 5
    if (keyframes.size() > 6)
    {
        float t5 = keyframes[5].time;
        float t6 = keyframes[6].time;

        if (animationTime >= t5 && animationTime < t6)
        {
            outPose = keyframes[5].boneTransforms;
            Logger::log("[DEBUG] Forcing direct frame 5 playback", Logger::WARNING);
            return;
        }
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

        glm::mat4 interp = interpolateMatrices(mat0, mat1, lerpFactor);

        if ((startFrame >= 1 && startFrame <= 8) &&
            (boneName.find("spine") != std::string::npos ||
                boneName.find("neck") != std::string::npos ||
                boneName.find("head") != std::string::npos ||
                boneName.find("thigh") != std::string::npos))
        {
            Logger::log("Runtime Interp [JAB HEAD]: Bone " + boneName +
                " from frame " + std::to_string(startFrame) +
                " to " + std::to_string(endFrame) +
                " | t=" + std::to_string(animationTime) +
                "\n" + glm::to_string(interp),
                Logger::WARNING);
        }

        outPose[boneName] = interp;
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


    // Generalized translation clamp: suppress jitter across all frames
    const float TRANSLATION_JUMP_THRESHOLD = 0.025f;
    const float ROTATION_JUMP_THRESHOLD = 10.0f;
    const size_t N = keyframes.size();

    for (size_t i = 1; i + 1 < N; ++i)
    {
        Keyframe& prev = keyframes[i - 1];
        Keyframe& curr = keyframes[i];
        Keyframe& next = keyframes[i + 1];

        for (const auto& [boneName, prevMat] : prev.boneTransforms)
        {
            if (!curr.boneTransforms.count(boneName) || !next.boneTransforms.count(boneName))
                continue;

            const glm::mat4& currMat = curr.boneTransforms[boneName];
            const glm::mat4& nextMat = next.boneTransforms[boneName];

            glm::vec3 prevT(prevMat[3]);
            glm::vec3 currT(currMat[3]);
            glm::vec3 nextT(nextMat[3]);

            if ((i >= 26 && i <= 28) || (i >= 57 && i <= 59))
            {
                if (boneName.find("thigh") != std::string::npos ||
                    boneName.find("shin") != std::string::npos ||
                    boneName.find("foot") != std::string::npos)
                {
                    float dx = glm::distance(prevT, currT);
                    float dy = glm::distance(currT, nextT);
                    float dSpan = glm::distance(prevT, nextT);

                    Logger::log("[DEBUG TRANSLATION] Bone " + boneName + " @frame=" + std::to_string(i) +
                        "\n    transPrev = " + glm::to_string(prevT) +
                        "\n    transCurr = " + glm::to_string(currT) +
                        "\n    transNext = " + glm::to_string(nextT) +
                        "\n    deltaPrev = " + std::to_string(dx) +
                        ", deltaNext = " + std::to_string(dy) +
                        ", deltaSpan = " + std::to_string(dSpan),
                        Logger::WARNING);
                }
            }

            float deltaPrev = glm::length(currT - prevT);
            float deltaNext = glm::length(currT - nextT);
            float deltaNeighbors = glm::length(nextT - prevT);

            bool isMiddleSpike =
                deltaPrev > TRANSLATION_JUMP_THRESHOLD &&
                deltaNext > TRANSLATION_JUMP_THRESHOLD &&
                deltaNeighbors < (0.5f * TRANSLATION_JUMP_THRESHOLD);

            bool isIsolatedJump =
                (deltaPrev > TRANSLATION_JUMP_THRESHOLD && deltaNext < (0.2f * TRANSLATION_JUMP_THRESHOLD)) ||
                (deltaNext > TRANSLATION_JUMP_THRESHOLD && deltaPrev < (0.2f * TRANSLATION_JUMP_THRESHOLD));

            bool isSmallNoise =
                deltaPrev > 0.002f &&
                deltaNext > 0.002f &&
                deltaNeighbors < 0.001f;

            bool shouldClamp = isMiddleSpike || isIsolatedJump || isSmallNoise;

            glm::vec3 scalePrev, scaleCurr, scaleNext;
            glm::quat rotPrev, rotCurr, rotNext;
            glm::vec3 transPrev, transCurr, transNext;
            glm::vec3 skewPrev, skewCurr, skewNext;
            glm::vec4 perspPrev, perspCurr, perspNext;

            glm::decompose(prevMat, scalePrev, rotPrev, transPrev, skewPrev, perspPrev);
            glm::decompose(currMat, scaleCurr, rotCurr, transCurr, skewCurr, perspCurr);
            glm::decompose(nextMat, scaleNext, rotNext, transNext, skewNext, perspNext);

            // Hemisphere flip fix (for Jab_Head full-body flip bug)
            if (glm::dot(rotPrev, rotCurr) < 0.0f && glm::dot(rotNext, rotCurr) < 0.0f)
            {
                rotCurr = -rotCurr;
                Logger::log("[HEMISPHERE FIX] Flipped rotCurr at frame " + std::to_string(i) + " for bone " + boneName, Logger::WARNING);
            }

            if (glm::dot(rotPrev, rotNext) < 0.0f)
                rotNext = -rotNext;

            float angleDeltaPrev = glm::degrees(glm::angle(glm::normalize(rotCurr) * glm::inverse(rotPrev)));
            float angleDeltaNext = glm::degrees(glm::angle(glm::normalize(rotCurr) * glm::inverse(rotNext)));
            float angleDeltaNeighbors = glm::degrees(glm::angle(glm::normalize(rotPrev) * glm::inverse(rotNext)));

            if (i >= 4 && i <= 6 && (boneName == "DEF-hips" || boneName == "root"))
            {
                Logger::log("[ROOT DEBUG] Bone '" + boneName + "' @frame=" + std::to_string(i) +
                    "\n    rot = " + glm::to_string(glm::normalize(rotCurr)) +
                    "\n    trans = " + glm::to_string(transCurr),
                    Logger::WARNING);

                Logger::log("    Angle delta (prev to curr): " + std::to_string(angleDeltaPrev), Logger::WARNING);
                Logger::log("    Angle delta (curr to next): " + std::to_string(angleDeltaNext), Logger::WARNING);
                Logger::log("    Angle delta (prev to next): " + std::to_string(angleDeltaNeighbors), Logger::WARNING);
            }

            bool isRotationSpike =
                angleDeltaPrev > ROTATION_JUMP_THRESHOLD &&
                angleDeltaNext > ROTATION_JUMP_THRESHOLD &&
                angleDeltaNeighbors < (0.5f * ROTATION_JUMP_THRESHOLD);

            if (isRotationSpike)
            {
                glm::quat smoothedR = glm::slerp(rotPrev, rotNext, 0.5f);
                glm::mat4 rotMat = glm::mat4_cast(smoothedR);
                glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scaleCurr);
                glm::mat4 transMat = glm::translate(glm::mat4(1.0f), transCurr);
                curr.boneTransforms[boneName] = transMat * rotMat * scaleMat;

                Logger::log("[FIXED - ROT SPIKE] Bone '" + boneName +
                    "' at frame " + std::to_string(i), Logger::WARNING);
            }

            if (shouldClamp)
            {
                glm::vec3 smoothedT = 0.5f * (prevT + nextT);
                glm::vec3 smoothedS = 0.5f * (scalePrev + scaleNext);

                if (glm::dot(rotPrev, rotNext) < 0.0f)
                    rotNext = -rotNext;

                glm::quat smoothedR = glm::slerp(glm::normalize(rotPrev), glm::normalize(rotNext), 0.5f);

                glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), smoothedS);
                glm::mat4 rotMat = glm::mat4_cast(smoothedR);
                glm::mat4 transMat = glm::translate(glm::mat4(1.0f), smoothedT);

                curr.boneTransforms[boneName] = transMat * rotMat * scaleMat;

                Logger::log("[FIXED - SRT+ROT] Bone '" + boneName +
                    "' at frame " + std::to_string(i), Logger::WARNING);
            }
        }
    }


    // === 5-frame smoothing ===
    for (size_t i = 2; i + 2 < N; ++i)
    {
        Keyframe& prev2 = keyframes[i - 2];
        Keyframe& curr = keyframes[i];
        Keyframe& next2 = keyframes[i + 2];

        for (const auto& [boneName, matPrev2] : prev2.boneTransforms)
        {
            if (!curr.boneTransforms.count(boneName) || !next2.boneTransforms.count(boneName))
                continue;

            const glm::mat4& matCurr = curr.boneTransforms[boneName];
            const glm::mat4& matNext2 = next2.boneTransforms[boneName];

            glm::vec3 scaleA, scaleB, scaleC;
            glm::quat rotA, rotB, rotC;
            glm::vec3 transA, transB, transC;
            glm::vec3 skew, skew2;
            glm::vec4 persp, persp2;

            glm::decompose(matPrev2, scaleA, rotA, transA, skew, persp);
            glm::decompose(matCurr, scaleB, rotB, transB, skew, persp);
            glm::decompose(matNext2, scaleC, rotC, transC, skew2, persp2);

            if (glm::dot(rotA, rotC) < 0.0f)
                rotC = -rotC;

            float deltaA = glm::degrees(glm::angle(glm::normalize(rotB) * glm::inverse(rotA)));
            float deltaC = glm::degrees(glm::angle(glm::normalize(rotB) * glm::inverse(rotC)));
            float arcAC = glm::degrees(glm::angle(glm::normalize(rotA) * glm::inverse(rotC)));

            if (deltaA > ROTATION_JUMP_THRESHOLD &&
                deltaC > ROTATION_JUMP_THRESHOLD &&
                arcAC < 0.5f * ROTATION_JUMP_THRESHOLD)
            {
                glm::quat smoothed = glm::slerp(rotA, rotC, 0.5f);
                glm::mat4 T = glm::translate(glm::mat4(1.0f), transB);
                glm::mat4 R = glm::mat4_cast(smoothed);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleB);

                curr.boneTransforms[boneName] = T * R * S;

                Logger::log("[FIXED - ROT ARC] Bone " + boneName +
                    " @frame=" + std::to_string(i), Logger::WARNING);
            }
        }
    }

    // === Specific root patch: Jab_Head, frame 5 ===
    if (name.find("Jab_Head") != std::string::npos && N > 6)
    {
        const std::string rootBone = "DEF-hips";
        Keyframe& prev = keyframes[4];
        Keyframe& curr = keyframes[5];
        Keyframe& next = keyframes[6];

        if (prev.boneTransforms.count(rootBone) &&
            curr.boneTransforms.count(rootBone) &&
            next.boneTransforms.count(rootBone))
        {
            glm::mat4 matPrev = prev.boneTransforms[rootBone];
            glm::mat4 matCurr = curr.boneTransforms[rootBone];
            glm::mat4 matNext = next.boneTransforms[rootBone];

            glm::vec3 s1, s2, s3;
            glm::quat q1, q2, q3;
            glm::vec3 t1, t2, t3;
            glm::vec3 skew;
            glm::vec4 persp;

            glm::decompose(matPrev, s1, q1, t1, skew, persp);
            glm::decompose(matCurr, s2, q2, t2, skew, persp);
            glm::decompose(matNext, s3, q3, t3, skew, persp);

            if (glm::dot(q1, q3) < 0.0f)
                q3 = -q3;

            float dPrev = glm::degrees(glm::angle(glm::normalize(q2) * glm::inverse(q1)));
            float dNext = glm::degrees(glm::angle(glm::normalize(q2) * glm::inverse(q3)));
            float dSpan = glm::degrees(glm::angle(glm::normalize(q1) * glm::inverse(q3)));

            if (dPrev > 90.0f && dNext > 90.0f && dSpan < 5.0f)
            {
                glm::quat smoothed = glm::slerp(q1, q3, 0.5f);
                glm::mat4 T = glm::translate(glm::mat4(1.0f), t2);
                glm::mat4 R = glm::mat4_cast(smoothed);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), s2);
                curr.boneTransforms[rootBone] = T * R * S;

                Logger::log("[PATCHED ROOT ROT FLIP] " + rootBone + " at frame 5 in Jab_Head", Logger::WARNING);
            }
        }
    }



    Logger::log("[INFO] Translation and rotation smoothing pass complete.", Logger::INFO);




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