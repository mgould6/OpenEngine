#ifndef ANIMATION_H
#define ANIMATION_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <glm/glm.hpp>

class Model;

/* Each keyframe is stored in SECONDS, not ticks */
struct Keyframe
{
    float time;                                          /* seconds */
    std::map<std::string, glm::mat4> boneTransforms;     /* local */
};

class Animation
{
public:
    explicit Animation(const std::string& filePath,
        const Model* model);

    /* status ---------------------------------------------------- */
    bool  isLoaded() const { return loaded; }
    bool  bindMismatchChecked() const { return mismatchChecked; }
    glm::mat4 getLocalMatrixAtTime(const std::string& bone,
        float seconds) const;
    /* timeline meta --------------------------------------------- */
    float getTicksPerSecond()   const { return ticksPerSecond; }
    float getDurationTicks()    const { return durationTicks; }
    float getClipDurationSeconds() const { return clipDurationSecs; } /* <-- restored */

    /* runtime --------------------------------------------------- */
    void  apply(float animationTimeSeconds, Model* model) const;
    void  interpolateKeyframes(float animationTimeSeconds,
        std::map<std::string, glm::mat4>& outPose) const;

    /* debug helpers --------------------------------------------- */
    size_t          getKeyframeCount() const { return keyframes.size(); }
    const std::string& getName() const { return name; }
    bool  mismatchChecked = false;
    void checkBindMismatch(const Model* model);
    const std::vector<Keyframe>& getKeyframes() const { return keyframes; }

private:
    /* helpers --------------------------------------------------- */
    void  loadAnimation(const std::string& filePath, const Model* model);
    glm::mat4 interpolateMatrices(const glm::mat4& a,
        const glm::mat4& b,
        float            factor) const;
    std::pair<size_t, size_t>
        findKeyframeIndices(float timeSeconds) const;

    /* data ------------------------------------------------------ */
    bool  loaded = false;
    float durationTicks = 0.0f;
    float ticksPerSecond = 24.0f;
    float clipDurationSecs = 0.0f;

    std::vector<Keyframe> keyframes;
    std::string           name;

    /* optional bookkeeping ------------------------------------- */
    std::vector<std::string> animatedBones;
};

#endif /* ANIMATION_H */
