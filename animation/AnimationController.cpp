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
    animationTime = 0.0f;

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
    // 0. early-out checks
    if (!currentAnimation)
    {
        Logger::log("ERROR: No current animation!", Logger::ERROR);
        return;
    }

    const float durationTicks = currentAnimation->getDuration();        // clip length (ticks)
    const float ticksPerSecond = currentAnimation->getTicksPerSecond();  // 60 for your exports

    if (durationTicks <= 0.0f || ticksPerSecond <= 0.0f)
    {
        Logger::log("ERROR: Invalid animation meta!", Logger::ERROR);
        return;
    }

    // 1. seconds -> fractional ticks
    float deltaTicks = deltaTime * ticksPerSecond;

    // Clamp unusually large advances (for example, first frame after a long stall)
    const float kMaxTickDelta = 2.0f;   // roughly two frames at 60 fps
    if (deltaTicks > kMaxTickDelta) deltaTicks = kMaxTickDelta;
    if (deltaTicks < -kMaxTickDelta) deltaTicks = -kMaxTickDelta;

    float newTime = animationTime + deltaTicks;

    // 2. wrap playhead into [0, duration)
    if (newTime >= durationTicks)
        newTime = fmodf(newTime, durationTicks);
    else if (newTime < 0.0f)
        newTime = durationTicks + fmodf(newTime, durationTicks); // handle rewinds

    // 3. diagnostics
    static float lastTime = -1.0f;
    if (lastTime >= 0.0f)
    {
        float diff = fabsf(newTime - lastTime);

        // Ignore the normal wrap-around jump at clip end
        if (diff > 1.2f && diff < durationTicks - 1.2f)
        {
            Logger::log("WARN  tick jump " +
                std::to_string(lastTime) + " -> " +
                std::to_string(newTime) +
                "  (d " + std::to_string(diff) + ")", Logger::WARNING);
        }
    }
    lastTime = newTime;
    animationTime = newTime;

    // 4. optional per-frame debug output
    Logger::log("CTRL  animTime = " +
        std::to_string(animationTime) + " ticks (" +
        std::to_string(deltaTicks) + " d ) / " +
        std::to_string(durationTicks), Logger::DEBUG);

    // pose application happens later in applyToModel()
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

    /* -------- 3. apply skinning transforms (keep full T-R-S) -------- */
    for (const auto& [boneName, globalTransform] : globalBoneMatrices)
    {
        glm::mat4 final =
            model->getGlobalInverseTransform()
            * globalTransform                         // no scale stripping
            * model->getBoneOffsetMatrix(boneName);

        model->setBoneTransform(boneName, final);
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
