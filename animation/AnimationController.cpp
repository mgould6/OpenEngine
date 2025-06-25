#define GLM_ENABLE_EXPERIMENTAL
#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "DebugTools.h"
#include <GLFW/glfw3.h>
#include <iomanip>   // for std::setw
#include <cmath>        // fmodf, fabsf
#include "Animation.h"


// Forward declaration ­
static glm::mat4 removeScale(const glm::mat4& m);


AnimationController::AnimationController(Model* model)
    : model(model)
    , currentAnimation(nullptr)
    , animationTime(0.0f)
{
}

/*  loadAnimation
    - If the clip name already exists and forceReload == true,
      delete the old Animation* and replace it with a fresh one.
----------------------------------------------------------------*/
bool AnimationController::loadAnimation(const std::string& name,
    const std::string& filePath,
    bool forceReload)
{
    auto it = animations.find(name);

    if (it != animations.end() && !forceReload)
        return true;                       // nothing to do

    if (it != animations.end())            // delete old copy
    {
        delete it->second;
        animations.erase(it);
    }

    Animation* clip = new Animation(filePath, model);
    if (!clip->isLoaded())
    {
        delete clip;
        return false;
    }
    animations[name] = clip;

    /* — NEW — if this clip is the current one, re-bind it immediately */
    if (currentAnimation == nullptr || currentAnimation->getName() == name)
    {
        currentAnimation = clip;
        animationTime = 0.0f;
        Logger::log("NOW PLAYING: " + name +
            "  keyframes=" + std::to_string(clip->getKeyframeCount()),
            Logger::INFO);
    }
    return true;
}
void AnimationController::setCurrentAnimation(const std::string& name)
{
    auto it = animations.find(name);
    if (it == animations.end())
    {
        Logger::log("Animation not found: " + name, Logger::ERROR);
        return;
    }

    currentAnimation = it->second;
    animationTime = 0.00001f;   /* avoid evaluating exactly at 0 */

    Logger::log("NOW PLAYING: " + name +
        "  keyframes=" +
        std::to_string(currentAnimation->getKeyframeCount()),
        Logger::INFO);
}


/*------------------------------------------------------------------
  Advance the animation clock
  ---------------------------------------------------------------
  - deltaTime arrives in seconds from the game loop.
  - We convert it to fractional Assimp "ticks":
        deltaTicks = deltaTime * ticksPerSecond;
  - animationTime is stored in ticks because the rest of the
    controller and applyToModel() expect that unit.
------------------------------------------------------------------*/
void AnimationController::update(float deltaTime)
{
    if (!currentAnimation)
    {
        Logger::log("ERROR: No current animation!", Logger::ERROR);
        return;
    }

    float clipSeconds = currentAnimation->getClipDurationSeconds();
    if (clipSeconds <= 0.0f)
    {
        Logger::log("ERROR: Invalid clip length!", Logger::ERROR);
        return;
    }

    animationTime += deltaTime;               /* seconds */

    /* wrap clock */
    if (animationTime >= clipSeconds)
        animationTime = std::fmod(animationTime, clipSeconds);
    else if (animationTime < 0.0f)
        animationTime = clipSeconds + std::fmod(animationTime, clipSeconds);
}

void AnimationController::applyToModel(Model* model)
{
    if (!model || !currentAnimation)
        return;

    /* -------- 1. interpolate local (animated) transforms -------- */
    std::map<std::string, glm::mat4> localBoneMatrices;
    currentAnimation->interpolateKeyframes(animationTime, localBoneMatrices);

    /* inject bind pose for any bones that lack keys */
    for (const auto& bone : model->getBones())
    {
        const std::string& boneName = bone.name;
        if (localBoneMatrices.find(boneName) == localBoneMatrices.end())
            localBoneMatrices[boneName] = model->getLocalBindPose(boneName);
    }

    /* -------- 2. recursively build global transforms ------------ */
    std::map<std::string, glm::mat4> globalBoneMatrices;
    for (const auto& [boneName, _] : localBoneMatrices)
        globalBoneMatrices[boneName] =
        buildGlobalTransform(boneName,
            localBoneMatrices,
            model,
            globalBoneMatrices);

    const glm::mat4 globalInverse = model->getGlobalInverseTransform();

    /* ------------------------------------------------------------------
       3. build and store the final skin matrices
          – global pose  : scale removed
          – bind pose    : scale removed
    ------------------------------------------------------------------ */
    for (const auto& pair : globalBoneMatrices)
    {
        const std::string& name = pair.first;
        const glm::mat4& globalScaled = pair.second;

        glm::mat4 final =
            model->getGlobalInverseTransform()
            * removeScale(globalScaled)                         // pose  (T * R)
            * model->getBoneOffsetMatrixNoScale(name);          // bind (no S)

        model->setBoneTransform(name, final);
    }

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
