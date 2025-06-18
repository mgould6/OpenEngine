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


/*---------------------------------------------------------------
    Advance the animation clock.
    – We ignore deltaTime and simply add ONE Assimp “tick”
      every rendered frame (engine runs at 60 Hz, clip is
      resampled to 60 fps, so 1 tick == 1 frame).
----------------------------------------------------------------*/
void AnimationController::update(float deltaTime)
{
    if (!currentAnimation)
    {
        Logger::log("ERROR: No current animation!", Logger::ERROR);
        return;
    }

    const float duration = currentAnimation->getDuration();      // ticks
    const float ticksPerSecond = currentAnimation->getTicksPerSecond();

    if (duration <= 0.0f || ticksPerSecond <= 0.0f)
    {
        Logger::log("ERROR: Invalid animation meta!", Logger::ERROR);
        return;
    }

    /* 1  advance clock in FRACTIONAL ticks */
    float deltaTicks = deltaTime * ticksPerSecond;
    animationTime += deltaTicks;

    /* 2  wrap at clip end */
    if (animationTime >= duration)
        animationTime = fmodf(animationTime, duration);

    /* 3  diagnostic: flag jumps bigger than 1.2 ticks */
    static float lastTime = -1.0f;
    if (lastTime >= 0.0f)
    {
        float diff = fabsf(animationTime - lastTime);
        if (diff > 1.2f)            // anything > 2 frames at 60 fps
        {
            Logger::log("WARN  tick jump " +
                std::to_string(lastTime) + " -> " +
                std::to_string(animationTime) +
                "  (Delta " + std::to_string(diff) + ")", Logger::WARNING);
        }
    }
    lastTime = animationTime;

    /* optional per-frame debug print */
    Logger::log("CTRL  animTime = " +
        std::to_string(animationTime) + " ticks (" +
        std::to_string(deltaTicks) + " Delta ) / " +
        std::to_string(duration), Logger::DEBUG);

    /* pose application happens later in applyToModel() */
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

    /* -------- 3. apply skinning transforms (strip scale BEFORE offset) --- */
    for (const auto& [boneName, globalTransform] : globalBoneMatrices)
    {
        /* 1) remove scale from the GLOBAL transform */
        glm::mat4 rotOnly = globalTransform;
        glm::vec3 x = glm::normalize(glm::vec3(rotOnly[0]));
        glm::vec3 y = glm::normalize(glm::vec3(rotOnly[1]));
        glm::vec3 z = glm::normalize(glm::vec3(rotOnly[2]));
        rotOnly[0] = glm::vec4(x, 0.0f);
        rotOnly[1] = glm::vec4(y, 0.0f);
        rotOnly[2] = glm::vec4(z, 0.0f);

        /* 2) apply offset AFTER we’ve removed scale */
        glm::mat4 offset = model->getBoneOffsetMatrix(boneName);
        glm::mat4 final = model->getGlobalInverseTransform() * rotOnly * offset;

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
