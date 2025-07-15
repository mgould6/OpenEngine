#define GLM_ENABLE_EXPERIMENTAL
#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include <fstream>
#include <glm/gtx/component_wise.hpp> // for glm::all(glm::equal …) style helpers
#include <glm/gtc/epsilon.hpp>  // epsilonEqual + all/any/not_ helpers
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>          // core
// optional but handy

#include "DebugTools.h"
#include <GLFW/glfw3.h>
#include <iomanip>   // for std::setw
#include <cmath>        // fmodf, fabsf
#include "Animation.h"
#include <cmath>          // std::fabs


// Forward declaration ­
static glm::mat4 removeScale(const glm::mat4& m);


AnimationController::AnimationController(Model* model)
    : model(model)
    , currentAnimation(nullptr)
    , animationTime(0.0f)
{
}


/* return true if every element differs by less than eps */
static bool matNearlyEqual(const glm::mat4& A,
    const glm::mat4& B,
    float            eps = 0.001f)
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (std::fabs(A[c][r] - B[c][r]) > eps)
                return false;
    return true;
}

/*--------------------------------------------------------------
    loadAnimation
    – Registers or reloads a clip and (optionally) makes it
      the active animation.
    – If forceReload == true and a clip with the same name
      already exists, the old one is deleted and replaced.
--------------------------------------------------------------*/
bool AnimationController::loadAnimation(const std::string& name,
    const std::string& filePath,
    bool              forceReload)
{
    /* ----------------------------------------------------------
       1.  Handle duplicates / hot-reloads
    ---------------------------------------------------------- */
    auto it = animations.find(name);

    if (it != animations.end() && !forceReload)
        return true;                       // nothing to do

    if (it != animations.end())            // replace old clip
    {
        delete it->second;
        animations.erase(it);
    }

    /* ----------------------------------------------------------
       2.  Load the new clip
    ---------------------------------------------------------- */
    Animation* clip = new Animation(filePath, model);
    if (!clip->isLoaded())
    {
        Logger::log("Failed to load animation: " + filePath,
            Logger::ERROR);
        delete clip;
        return false;
    }
    animations[name] = clip;

    /* ----------------------------------------------------------
       3.  Auto-bind if this is the selected clip
           (or if nothing is currently playing)
    ---------------------------------------------------------- */
    if (currentAnimation == nullptr || currentAnimation->getName() == name)
    {
        currentAnimation = clip;

        // UPDATE: start slightly after 0 to avoid sampling the bind pose
        animationTime = 1e-5f;

        Logger::log("NOW PLAYING: " + name +
            " | keyframes = " +
            std::to_string(clip->getKeyframeCount()) +
            " | length = " +
            std::to_string(clip->getClipDurationSeconds()) + " s",
            Logger::INFO);
    }

    /* ----------------------------------------------------------
   OPTIONAL: verify frame 0 pose matches the model bind pose
---------------------------------------------------------- */
    if (clip->isLoaded())
    {
        std::map<std::string, glm::mat4> firstPose;
        clip->interpolateKeyframes(1e-6f, firstPose);   // ~frame 0

        for (const auto& name : { "DEF-spine", "DEF-thigh.L", "DEF-upper_arm.L" }) {
            if (firstPose.count(name)) {
                Logger::log("POSE AT t=0 FOR " + std::string(name) + ":\n" + glm::to_string(firstPose[name]), Logger::WARNING);
            }
            else {
                Logger::log("Pose missing for bone: " + std::string(name), Logger::WARNING);
            }

            glm::mat4 bindLocal = model->getLocalBindPose(name);
            Logger::log("BIND POSE LOCAL FOR " + std::string(name) + ":\n" + glm::to_string(bindLocal), Logger::WARNING);
        }

        bool poseMatchesBind = true;

        for (const auto& bone : model->getBones())
        {
            const std::string& name = bone.name;
            glm::mat4 bindLocal = model->getLocalBindPose(name);
            glm::mat4 poseLocal =
                firstPose.count(name) ? firstPose[name] : bindLocal;

            if (!matNearlyEqual(bindLocal, poseLocal, 1e-5f))
            {
                glm::vec3 tBind(bindLocal[3]);
                glm::vec3 tPose(poseLocal[3]);
                glm::vec3 delta = tPose - tBind;

                Logger::log("[MISMATCH] Bone '" + name + "' differs in frame 0", Logger::WARNING);
                Logger::log("  Bind  T: " + glm::to_string(tBind), Logger::WARNING);
                Logger::log("  Frame0 T: " + glm::to_string(tPose), Logger::WARNING);
                Logger::log("  Delta Translation: " + glm::to_string(delta), Logger::WARNING);

                poseMatchesBind = false;
            }

        }

        if (poseMatchesBind)
            Logger::log("[OK] First frame of '" + name +
                "' matches bind pose.", Logger::INFO);
    }



    return true;
}

void AnimationController::setCurrentAnimation(const std::string& name)
{
    auto it = animations.find(name);
    if (it == animations.end())
    {
        Logger::log("ERROR: Animation not found: " + name, Logger::ERROR);
        return;
    }

    Animation* newClip = it->second;

    // Avoid resetting if this is already the active animation
    if (currentAnimation == newClip)
    {
        Logger::log("INFO: Animation [" + name + "] is already playing.", Logger::INFO);
        return;
    }

    // Run bind mismatch check only once per clip
    if (!newClip->bindMismatchChecked())
        newClip->checkBindMismatch(model);

    currentAnimation = newClip;
    animationTime = 0.00001f;  // Ensure we skip t=0 precision issues

    Logger::log("NOW PLAYING: [" + name + "]"
        "  keyframes=" + std::to_string(currentAnimation->getKeyframeCount()) +
        "  duration=" + std::to_string(currentAnimation->getClipDurationSeconds()) + "s",
        Logger::INFO);
}




const glm::mat4& AnimationController::bindGlobalNoScale
(const std::string& bone) const
{
    static const glm::mat4 I(1.0f);
    if (!model || !model->getBindPose())
        return I;
    return model->getBindPose()->getGlobalNoScale(bone);
}



/*------------------------------------------------------------------
  - deltaTime arrives in seconds from the game loop.
  - We convert it to fractional Assimp "ticks":
        deltaTicks = deltaTime * ticksPerSecond;
  - animationTime is stored in ticks because the rest of the
    controller and applyToModel() expect that unit.
------------------------------------------------------------------*/
/*------------------------------------------------------------------
  Advance the animation clock – seconds domain
------------------------------------------------------------------*/
void AnimationController::update(float deltaTime)
{
    if (!currentAnimation)
        return;

    /* clamp huge pauses (alt-tab, breakpoint) */
    const float MAX_DT = 0.05f;          /* 20 fps floor */
    if (deltaTime > MAX_DT)
        deltaTime = MAX_DT;

    animationTime += deltaTime;          /* seconds */

    float clipSeconds = currentAnimation->getClipDurationSeconds();
    if (clipSeconds <= 0.0f)
        return;

    /*  wrap, but never sample exactly at 0 or clipSeconds  */
    const float EPS = 1e-4f;             /* 0.0001 s */
    if (animationTime >= clipSeconds - EPS)
        animationTime = EPS;             /* restart *after* 0 */

    if (animationTime < EPS)
        animationTime = EPS;
}

void AnimationController::applyToModel(Model* model)
{
    if (!model || !currentAnimation) return;

    /* 1. local-pose interpolation (unchanged) */
    std::map<std::string, glm::mat4> localBoneMatrices;
    currentAnimation->interpolateKeyframes(animationTime, localBoneMatrices);
    Logger::log("Applied animation frame at t=" + std::to_string(animationTime), Logger::INFO);

    for (const auto& bone : model->getBones())
        if (!localBoneMatrices.count(bone.name))
            localBoneMatrices[bone.name] = model->getLocalBindPose(bone.name);

    /* 2. build global transforms (unchanged) */
    std::map<std::string, glm::mat4> globalBoneMatrices;
    for (const auto& [boneName, _] : localBoneMatrices)
        globalBoneMatrices[boneName] =
        buildGlobalTransform(boneName, localBoneMatrices, model, globalBoneMatrices);

    /* 3. final skin matrices */
    static bool dumpOnce = true;          // <-- persists across frames
    for (const auto& [boneName, globalScaled] : globalBoneMatrices)
    {
        glm::mat4 offsetMatrix = model->getBoneOffsetMatrix(boneName);
        glm::mat4 final =
            model->getGlobalInverseTransform()
            * globalScaled
            * offsetMatrix;


        if (dumpOnce)
        {
            Logger::log("Bone [" + boneName + "]", Logger::INFO);
            Logger::log("Bind (no scale):\n" +
                glm::to_string(model->getBoneOffsetMatrixNoScale(boneName)), Logger::INFO);
            Logger::log("Pose (no scale):\n" +
                glm::to_string(removeScale(globalScaled)), Logger::INFO);
            Logger::log("Final:\n" + glm::to_string(final), Logger::INFO);
        }
        model->setBoneTransform(boneName, final);
    }
    dumpOnce = false;
}



glm::mat4 AnimationController::buildGlobalTransform(
    const std::string& boneName,
    const std::map<std::string, glm::mat4>& localBoneMatrices,
    Model* model,
    std::map<std::string, glm::mat4>& globalBoneMatrices
)
{
    // Already computed?
    if (globalBoneMatrices.find(boneName) != globalBoneMatrices.end())
        return globalBoneMatrices[boneName];

    // Get parent's global transform
    glm::mat4 parentGlobal = glm::mat4(1.0f);
    std::string parentName = model->getBoneParent(boneName);
    if (!parentName.empty())
        parentGlobal = buildGlobalTransform(parentName, localBoneMatrices, model, globalBoneMatrices);

    // Get animated local transform if available
    glm::mat4 animatedLocal = glm::mat4(1.0f);
    auto it = localBoneMatrices.find(boneName);
    if (it != localBoneMatrices.end())
        animatedLocal = it->second;
    else
        animatedLocal = model->getLocalBindPose(boneName);  // fallback to bind pose local if no animation!

    // Build global transform
    glm::mat4 globalTransform = parentGlobal * animatedLocal;

    // Cache
    globalBoneMatrices[boneName] = globalTransform;
    return globalTransform;
}




bool AnimationController::isAnimationPlaying() const
{
    return currentAnimation != nullptr;
}

void AnimationController::stopAnimation()
{
    currentAnimation = nullptr;
    animationTime = 0.0f;
    Logger::log("Animation stopped.", Logger::INFO);
}

void AnimationController::resetAnimation()
{
    animationTime = 0.0f;
}


// Remove scale from a TRS matrix but keep translation & rotation
static glm::mat4 removeScale(const glm::mat4& m)
{
    // 1. extract translation (x,y,z from the 4th column)
    glm::vec3 t(m[3].x, m[3].y, m[3].z);

    // 2. extract pure rotation (quat kills scale/shear)
    glm::quat r = glm::quat_cast(m);
    glm::mat4 out = glm::mat4_cast(r);

    // 3. re-attach translation
    out[3] = glm::vec4(t, 1.0f);
    return out;
}
