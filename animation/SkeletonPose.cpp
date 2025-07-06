#include "SkeletonPose.h"
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

SkeletonPose::SkeletonPose(
    const std::unordered_map<std::string, glm::mat4>& localBind,
    const std::unordered_map<std::string, glm::mat4>& globalBind,
    const std::unordered_map<std::string, int>& boneMap)
{
    for (const auto& p : globalBind)
        invGlobalNoScale[p.first] = glm::inverse(removeScale(p.second));
}

const glm::mat4& SkeletonPose::getGlobalNoScale
(const std::string& bone) const
{
    static const glm::mat4 I(1.0f);
    auto it = invGlobalNoScale.find(bone);
    return (it != invGlobalNoScale.end()) ? it->second : I;
}


// Static method definition
glm::mat4 SkeletonPose::removeScale(const glm::mat4& m)
{
    glm::vec3 t(m[3].x, m[3].y, m[3].z);           // Extract translation
    glm::quat r = glm::quat_cast(m);              // Extract rotation
    glm::mat4 out = glm::mat4_cast(r);
    out[3] = glm::vec4(t, 1.0f);
    return out;
}