#ifndef SKELETON_POSE_H
#define SKELETON_POSE_H

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

class SkeletonPose
{
public:
    SkeletonPose(
        const std::unordered_map<std::string, glm::mat4>& localBind,
        const std::unordered_map<std::string, glm::mat4>& globalBind,
        const std::unordered_map<std::string, int>& boneMap);

    // TR-only inverse bind (scale stripped)
    const glm::mat4& getGlobalNoScale(const std::string& bone) const;
    static glm::mat4 removeScale(const glm::mat4& m);

private:
    std::unordered_map<std::string, glm::mat4> invGlobalNoScale;
};

#endif // SKELETON_POSE_H
