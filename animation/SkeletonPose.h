#ifndef SKELETON_POSE_H
#define SKELETON_POSE_H

#include <unordered_map>
#include <map>
#include <string>
#include <glm/glm.hpp>

class Model; // Forward declare Model to avoid circular include

class SkeletonPose
{
public:
    SkeletonPose() = default;

    SkeletonPose(
        const std::unordered_map<std::string, glm::mat4>& localBind,
        const std::unordered_map<std::string, glm::mat4>& globalBind,
        const std::unordered_map<std::string, int>& boneMap);

    static SkeletonPose fromAnimationSample(
        const std::map<std::string, glm::mat4>& localBoneTransforms,
        const Model* model);

    const glm::mat4& getGlobalNoScale(const std::string& bone) const;

    static glm::mat4 removeScale(const glm::mat4& m);

    std::unordered_map<std::string, glm::mat4> boneTransforms;

private:
    std::unordered_map<std::string, glm::mat4> invGlobalNoScale;
};

#endif // SKELETON_POSE_H
