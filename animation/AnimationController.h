#pragma once
#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H

#include <string>
#include <unordered_map>
#include <map>
#include <glm/glm.hpp>
#include "../model/Model.h"
#include "Animation.h"

class AnimationController {
public:
    explicit AnimationController(Model* model);

    /* load (or reload) a clip  
       – if forceReload == true and a clip with the same name
         already exists, the old one is deleted and re-loaded       */
    bool loadAnimation(const std::string& name,
        const std::string& filePath,
        bool forceReload = false);

    /* bind a previously loaded clip for playback                   */
    void setCurrentAnimation(const std::string& name);

    /* advance the clock – now one tick per rendered frame          */
    void update(float deltaTime);

    /* push the current pose into the model for GPU skinning        */
    void applyToModel(Model* model);

    bool isAnimationPlaying() const;
    void stopAnimation();

    bool isClipLoaded(const std::string& name) const
    {
        return animations.find(name) != animations.end();
    }

private:
    Model* model;                                           // target model
    std::unordered_map<std::string, Animation*> animations; // loaded clips
    Animation* currentAnimation = nullptr;                  // playing clip
    float animationTime = 0.0f;                             // tick clock

    void resetAnimation();

    /* hierarchy helper */
    glm::mat4 buildGlobalTransform(
        const std::string& boneName,
        const std::map<std::string, glm::mat4>& localBoneMatrices,
        Model* model,
        std::map<std::string, glm::mat4>& globalBoneMatrices);


};

#endif // ANIMATIONCONTROLLER_H

