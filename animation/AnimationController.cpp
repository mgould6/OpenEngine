#define GLM_ENABLE_EXPERIMENTAL
#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "DebugTools.h"
#include <GLFW/glfw3.h>

// Include your "Animation.h" if needed, or forward declare
// #include "Animation.h"

AnimationController::AnimationController(Model* model)
    : model(model)
    , currentAnimation(nullptr)
    , animationTime(0.0f)
{
}

bool AnimationController::loadAnimation(const std::string& name, const std::string& filePath)
{
    if (animations.find(name) != animations.end())
    {
        Logger::log("Animation already loaded: " + name, Logger::WARNING);
        return false;
    }

    Logger::log("INFO: Attempting to load animation: " + name + " from file: " + filePath, Logger::INFO);

    std::ifstream file(filePath);
    if (!file.good())
    {
        Logger::log("ERROR: Animation file not found: " + filePath, Logger::ERROR);
        return false;
    }

    Animation* animation = new Animation(filePath, model);
    if (!animation->isLoaded())
    {
        Logger::log("ERROR: Failed to load animation data from file: " + filePath, Logger::ERROR);
        delete animation;
        return false;
    }

    animations[name] = animation;
    Logger::log("INFO: Successfully loaded animation: " + name, Logger::INFO);
    return true;
}

void AnimationController::setCurrentAnimation(const std::string& name)
{
    if (animations.find(name) != animations.end())
    {
        currentAnimation = animations[name];
        resetAnimation();
        Logger::log("Switched to animation: " + name, Logger::INFO);
    }
    else
    {
        Logger::log("Animation not found: " + name, Logger::ERROR);
    }
}

void AnimationController::update(float deltaTime)
{
    if (!currentAnimation)
    {
        Logger::log("ERROR: No current animation set in AnimationController!", Logger::ERROR);
        return;
    }

    if (currentAnimation->getDuration() <= 0.0f)
    {
        Logger::log("ERROR: Animation duration is zero or invalid!", Logger::ERROR);
        return;
    }

    animationTime += deltaTime;
    animationTime = fmod(animationTime, currentAnimation->getDuration());

    Logger::log("Debug: Animation time updated to: " + std::to_string(animationTime), Logger::INFO);
}

void AnimationController::applyToModel(Model* model)
{
    if (!model || !currentAnimation)
        return;

    // Step 1: Interpolate local (animated) transforms for each bone
    std::map<std::string, glm::mat4> localBoneMatrices;
    float currentTime = animationTime;
    currentAnimation->interpolateKeyframes(currentTime, localBoneMatrices);

    // Step 2: Recursively compute global transforms
    std::map<std::string, glm::mat4> globalBoneMatrices;

    for (const auto& [boneName, _] : localBoneMatrices)
    {
        buildGlobalTransform(boneName, localBoneMatrices, model, globalBoneMatrices);
    }

    // Retrieve the global inverse transform from Model (critical step!)
    glm::mat4 globalInverseTransform = model->getGlobalInverseTransform();

    // Step 3: Apply final skinning transforms to model
    for (const auto& [boneName, globalTransform] : globalBoneMatrices)
    {
        glm::mat4 offsetMatrix = model->getBoneOffsetMatrix(boneName);

        glm::mat4 finalTransform = globalInverseTransform * globalTransform * offsetMatrix;

        // Explicit logging of final calculated transforms
        Logger::log("DEBUG: Final (GlobalInverse * Global * Offset) Transform for bone [" + boneName + "]:\n" +
            glm::to_string(finalTransform), Logger::WARNING);

        // Diagnostic: Target specific bones for detailed inspection
        if (boneName == "DEF-upper_arm.L" || boneName == "DEF-forearm.L")
        {
            Logger::log("=== DEBUG: FINAL TRANSFORM APPLY FOR " + boneName + " ===", Logger::WARNING);
            Logger::log("Global Transform:\n" + glm::to_string(globalTransform), Logger::WARNING);
            Logger::log("Offset Matrix (Inverse Bind Pose):\n" + glm::to_string(offsetMatrix), Logger::WARNING);
            Logger::log("Global Inverse Transform:\n" + glm::to_string(globalInverseTransform), Logger::WARNING);
            Logger::log("Resulting Final Transform:\n" + glm::to_string(finalTransform), Logger::WARNING);

            DebugTools::logDecomposedTransform(boneName, finalTransform);
        }

        model->setBoneTransform(boneName, finalTransform);
        Logger::log("Applied globalInverse * global * offset for bone: " + boneName, Logger::INFO);
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
