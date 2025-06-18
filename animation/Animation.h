#ifndef ANIMATION_H
#define ANIMATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <glm/glm.hpp>

#include "../model/Model.h"

/* ------------------------------------------------------------------
   A single point on the timeline
   ------------------------------------------------------------------ */
struct Keyframe
{
    float timestamp;                                   // Assimp "ticks"
    std::map<std::string, glm::mat4> boneTransforms;   // local space
};

class Animation
{
public:
    explicit Animation(const std::string& filePath, const Model* model);

    /* state -------------------------------------------------------- */
    bool  isLoaded() const { return loaded; }

    /* timeline meta ------------------------------------------------ */
    float getTicksPerSecond()  const { return ticksPerSecond; }
    float getDurationTicks()   const { return duration; }
    float getDurationSeconds() const
    {
        return (ticksPerSecond > 0.0f) ? duration / ticksPerSecond : 0.0f;
    }

    /* legacy (controller still calls this) ------------------------ */
    float getDuration() const { return duration; }

    /* playback ----------------------------------------------------- */
    void  apply(float animationTimeTicks, Model* model);
    void  interpolateKeyframes(float animationTimeTicks,
        std::map<std::string, glm::mat4>&
        finalBoneMatrices) const;

    const std::string& getName()           const { return name; }
    size_t             getKeyframeCount()  const { return keyframes.size(); }


private:
    /* data loaded from file --------------------------------------- */
    bool  loaded = false;
    float duration = 0.0f;   // ticks
    float ticksPerSecond = 24.0f;  // fallback

    std::vector<Keyframe> keyframes;
    std::string name;



    /* helpers ------------------------------------------------------ */
    void        loadAnimation(const std::string& filePath,
        const Model* model);
    glm::mat4   interpolateMatrices(const glm::mat4& m1,
        const glm::mat4& m2,
        float factor) const;      // <- name fixed

    /* bookkeeping -------------------------------------------------- */
    std::vector<std::string> animatedBones;
    std::unordered_map<float, std::map<std::string, glm::mat4>>
        timestampToBoneMap;
    std::unordered_set<std::string> bonesWithKeyframes;
};

#endif /* ANIMATION_H */
