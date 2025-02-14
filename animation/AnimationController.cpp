#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include "../shaders/ShaderManager.h"
#include <map>
#include <string>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

AnimationController::AnimationController(Model* model)
    : model(model), currentAnimation(nullptr), animationTime(0.0f) {}

bool AnimationController::loadAnimation(const std::string& name, const std::string& filePath) {
    if (animations.find(name) != animations.end()) {
        Logger::log("Animation already loaded: " + name, Logger::WARNING);
        return false;
    }

    Logger::log("INFO: Attempting to load animation: " + name + " from file: " + filePath, Logger::INFO);

    std::ifstream file(filePath);
    if (!file.good()) {
        Logger::log("ERROR: Animation file not found: " + filePath, Logger::ERROR);
        return false;
    }

    Animation* animation = new Animation(filePath);
    if (!animation->isLoaded()) {
        Logger::log("ERROR: Failed to load animation data from file: " + filePath, Logger::ERROR);
        delete animation;
        return false;
    }

    animations[name] = animation;
    Logger::log("INFO: Successfully loaded animation: " + name, Logger::INFO);
    return true;
}

void AnimationController::setCurrentAnimation(const std::string& name) {
    if (animations.find(name) != animations.end()) {
        currentAnimation = animations[name];
        resetAnimation();
        Logger::log("Switched to animation: " + name, Logger::INFO);
    }
    else {
        Logger::log("Animation not found: " + name, Logger::ERROR);
    }
}

void AnimationController::update(float deltaTime) {
    if (!currentAnimation) {
        Logger::log("ERROR: No current animation set in AnimationController!", Logger::ERROR);
        return;
    }

    if (currentAnimation->getDuration() <= 0.0f) {
        Logger::log("ERROR: Animation duration is zero or invalid!", Logger::ERROR);
        return;
    }

    animationTime += deltaTime;
    animationTime = fmod(animationTime, currentAnimation->getDuration());

    Logger::log("Debug: Animation time updated to: " + std::to_string(animationTime), Logger::INFO);
}

// AnimationController.cpp
// AnimationController.cpp
void AnimationController::applyToModel(Model* model)
{
    if (!model || !currentAnimation) return;

    std::map<std::string, glm::mat4> localBoneMatrices;
    float currentTime = animationTime;

    // Interpolate keyframes => each bone gets its local transform from the FBX data
    currentAnimation->interpolateKeyframes(currentTime, localBoneMatrices);

    // Store these local transforms directly; do NOT multiply parents here
    for (const auto& [boneName, localTransform] : localBoneMatrices)
    {
        // Just set the bone’s local transform (no parent recursion)
        model->setBoneTransform(boneName, localTransform);

        // Optional debug logs
        if (boneName == "DEF-breast.L" || boneName == "DEF-shoulder.L")
        {
            Logger::log("DEBUG: Local Bone Transform - " + boneName, Logger::INFO);
            for (int i = 0; i < 4; i++)
            {
                Logger::log(
                    std::to_string(localTransform[i][0]) + " " +
                    std::to_string(localTransform[i][1]) + " " +
                    std::to_string(localTransform[i][2]) + " " +
                    std::to_string(localTransform[i][3]),
                    Logger::INFO
                );
            }
        }
    }
}

bool AnimationController::isAnimationPlaying() const {
    return currentAnimation != nullptr;
}

void AnimationController::stopAnimation() {
    currentAnimation = nullptr;
    animationTime = 0.0f;
    Logger::log("Animation stopped.", Logger::INFO);
}

void AnimationController::resetAnimation() {
    animationTime = 0.0f;
}
