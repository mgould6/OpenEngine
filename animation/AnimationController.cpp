#include "AnimationController.h"
#include "../common_utils/Logger.h"

AnimationController::AnimationController(Model* model)
    : model(model), currentAnimation(nullptr), animationTime(0.0f) {}

bool AnimationController::loadAnimation(const std::string& name, const std::string& filePath) {
    if (animations.find(name) != animations.end()) {
        Logger::log("Animation already loaded: " + name, Logger::WARNING);
        return false;
    }

    Animation* animation = new Animation(filePath);
    if (!animation->isLoaded()) {
        Logger::log("Failed to load animation: " + filePath, Logger::ERROR);
        delete animation;
        return false;
    }

    animations[name] = animation;
    Logger::log("Loaded animation: " + name, Logger::INFO);
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
}

void AnimationController::applyToModel(Model* model) {
    if (currentAnimation) {
        currentAnimation->apply(animationTime, model);
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
