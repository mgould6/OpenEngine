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

void AnimationController::applyToModel(Model* model) {
    if (!model || !currentAnimation) {
        Logger::log("ERROR: Animation is not loaded or has no keyframes!", Logger::ERROR);
        return;
    }

    float animationTime = fmod(glfwGetTime() * currentAnimation->getTicksPerSecond(), currentAnimation->getDuration());

    std::map<std::string, glm::mat4> finalBoneMatrices;
    currentAnimation->interpolateKeyframes(animationTime, finalBoneMatrices);

    std::vector<glm::mat4> boneTransforms(100, glm::mat4(1.0f));
    for (const auto& pair : finalBoneMatrices) {
        int boneIndex = model->getBoneIndex(pair.first);
        if (boneIndex >= 0 && boneIndex < 100) {
            boneTransforms[boneIndex] = pair.second;

            // **Logging the applied bone transformations**
            Logger::log("DEBUG: Bone " + pair.first + " Transform:", Logger::INFO);
            for (int i = 0; i < 4; i++) {
                Logger::log(
                    std::to_string(pair.second[i][0]) + " " +
                    std::to_string(pair.second[i][1]) + " " +
                    std::to_string(pair.second[i][2]) + " " +
                    std::to_string(pair.second[i][3]), Logger::INFO);
            }
        }
    }

    if (ShaderManager::boneShader) {
        glUniformMatrix4fv(glGetUniformLocation(ShaderManager::boneShader->ID, "boneTransforms"), 100, GL_FALSE, glm::value_ptr(boneTransforms[0]));
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
