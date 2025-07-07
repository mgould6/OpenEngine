#include "SkeletonPose.h"
#define GLM_ENABLE_EXPERIMENTAL

#include "../common_utils/Logger.h"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../model/Model.h"

SkeletonPose::SkeletonPose(
    const std::unordered_map<std::string, glm::mat4>& localBind,
    const std::unordered_map<std::string, glm::mat4>& globalBind,
    const std::unordered_map<std::string, int>& boneMap)
{
    for (const auto& p : globalBind)
        invGlobalNoScale[p.first] = glm::inverse(removeScale(p.second));
}

SkeletonPose SkeletonPose::fromAnimationSample(
    const std::map<std::string, glm::mat4>& localBoneTransforms,
    const Model* model)
{
    SkeletonPose pose;

    for (const auto& [boneName, localTransform] : localBoneTransforms)
    {
        glm::mat4 parentGlobal = glm::mat4(1.0f);
        std::string parent = model->getBoneParent(boneName);

        if (!parent.empty())
        {
            auto it = pose.boneTransforms.find(parent);
            if (it != pose.boneTransforms.end())
            {
                parentGlobal = it->second;
            }
            else
            {
                parentGlobal = model->getBindPoseGlobalTransform(parent);
            }
        }

        glm::mat4 globalTransform = parentGlobal * localTransform;
        pose.boneTransforms[boneName] = globalTransform;

        if (boneName == "DEF-forearm.L" || boneName == "DEF-upper_arm.L")
        {
            Logger::log("POSE DEBUG: [" + boneName + "] Global = " + glm::to_string(globalTransform), Logger::WARNING);
        }
    }

    return pose;
}

const glm::mat4& SkeletonPose::getGlobalNoScale(const std::string& bone) const
{
    static const glm::mat4 I(1.0f);
    auto it = invGlobalNoScale.find(bone);
    return (it != invGlobalNoScale.end()) ? it->second : I;
}

glm::mat4 SkeletonPose::removeScale(const glm::mat4& m)
{
    glm::vec3 t(m[3].x, m[3].y, m[3].z);
    glm::quat r = glm::quat_cast(m);
    glm::mat4 out = glm::mat4_cast(r);
    out[3] = glm::vec4(t, 1.0f);
    return out;
}
