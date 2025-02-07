#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include <fstream>  

AnimationController::AnimationController(Model* model)
    : model(model), currentAnimation(nullptr), animationTime(0.0f) {}

bool AnimationController::loadAnimation(const std::string& name, const std::string& filePath) {
    if (animations.find(name) != animations.end()) {
        Logger::log("Animation already loaded: " + name, Logger::WARNING);
        return false;
    }

    Logger::log("INFO: Attempting to load animation: " + name + " from file: " + filePath, Logger::INFO);

    // Debug: Verify file existence
    std::ifstream file(filePath);  // Fix: Ensure <fstream> is included
    if (!file.good()) {  // Fix: Use .good() instead of checking object directly
        Logger::log("ERROR: Animation file not found: " + filePath, Logger::ERROR);
        return false;
    }
    else {
        Logger::log("INFO: Animation file exists. Proceeding with loading.", Logger::INFO);
    }

    // Create Animation instance
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
    if (!currentAnimation) return;

    animationTime += deltaTime;

    // Loop the animation if it exceeds its duration
    if (animationTime > currentAnimation->getDuration()) {
        animationTime = std::fmod(animationTime, currentAnimation->getDuration());
    }

    Logger::log("Debug: Animation time updated to: " + std::to_string(animationTime), Logger::INFO);

}

void AnimationController::applyToModel(Model* model) {
    if (currentAnimation) {
        currentAnimation->apply(animationTime, model);
        Logger::log("Debug: Applying animation to model at time: " + std::to_string(animationTime), Logger::INFO);

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
