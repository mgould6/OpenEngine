#pragma once
#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H

#include <string>
#include <unordered_map>
#include "../model/Model.h"
#include "Animation.h"

class AnimationController {
public:
    explicit AnimationController(Model* model);

    // Load an animation from file and associate it with a name
    bool loadAnimation(const std::string& name, const std::string& filePath);

    // Set the current animation to play by its name
    void setCurrentAnimation(const std::string& name);

    // Update the animation based on deltaTime
    void update(float deltaTime);

    // Apply the current animation to the model
    void applyToModel(Model* model);

    // Check if an animation is currently playing
    bool isAnimationPlaying() const;

    // Stop the current animation
    void stopAnimation();

private:
    Model* model; // The model controlled by this animation system
    std::unordered_map<std::string, Animation*> animations; // Loaded animations mapped by name
    Animation* currentAnimation; // Currently playing animation
    float animationTime; // Current time in the animation playback

    void resetAnimation(); // Reset the animation to its start
};

#endif // ANIMATIONCONTROLLER_H
