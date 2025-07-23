#pragma once
#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "../model/Model.h"
#include "Animation.h"
#include "SkeletonPose.h"

class AnimationController {
public:
    explicit AnimationController(Model* model);

    bool loadAnimation(const std::string& name,
        const std::string& filePath,
        bool forceReload = false);

    void setCurrentAnimation(const std::string& name);

    void update(float deltaTime);
    void applyToModel(Model* model);

    bool isAnimationPlaying() const;
    void stopAnimation();

    bool isClipLoaded(const std::string& name) const {
        return animations.find(name) != animations.end();
    }

    const std::vector<Keyframe>& getKeyframes() const {
        return currentAnimation ? currentAnimation->getKeyframes() : emptyKeyframeList;
    }

    const std::string& getCurrentAnimationName() const {
        static std::string none = "None";
        return currentAnimation ? currentAnimation->getName() : none;
    }

    int getFrameCount() const {
        return currentAnimation ? static_cast<int>(currentAnimation->getKeyframes().size()) : 0;
    }

    // Debug playback flags
    bool debugPlay = true;
    bool debugStep = false;
    bool debugRewind = false;
    int debugFrame = 0;

private:
    Model* model;
    std::unordered_map<std::string, Animation*> animations;
    Animation* currentAnimation = nullptr;
    float animationTime = 0.0f;

    void resetAnimation();

    glm::mat4 buildGlobalTransform(
        const std::string& boneName,
        const std::map<std::string, glm::mat4>& localBoneMatrices,
        Model* model,
        std::map<std::string, glm::mat4>& globalBoneMatrices);
    const glm::mat4& bindGlobalNoScale(const std::string& bone) const;

    inline static const std::vector<Keyframe> emptyKeyframeList = {};
};

#endif // ANIMATIONCONTROLLER_H
