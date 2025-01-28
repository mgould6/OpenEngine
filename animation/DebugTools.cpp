// Updated DebugTools.cpp
#define GLM_ENABLE_EXPERIMENTAL
#include "DebugTools.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include "../common_utils/Logger.h"

namespace DebugTools {

    void renderBoneHierarchy(Model* model, const Camera& camera) {
        if (!model) return;

        const auto& boneTransforms = model->getBoneTransforms();
        const auto& bones = model->getBones();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.ProjectionMatrix;

        for (const auto& bone : bones) {
            auto it = boneTransforms.find(bone.name);
            if (it == boneTransforms.end()) continue;

            const glm::mat4& boneWorldTransform = it->second;
            glm::vec4 bonePosition = boneWorldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            Logger::log("Bone: " + bone.name + " Position: (" +
                std::to_string(bonePosition.x) + ", " +
                std::to_string(bonePosition.y) + ", " +
                std::to_string(bonePosition.z) + ")", Logger::INFO);
        }
    }

    void renderSkeleton(Model* model, const glm::mat4& view, const glm::mat4& projection) {
        if (!model) return;

        const auto& bones = model->getBones();
        for (const auto& bone : bones) {
            if (!bone.parentName.empty()) {
                const glm::mat4& parentTransform = model->getBoneTransform(bone.parentName);
                const glm::mat4& childTransform = model->getBoneTransform(bone.name);

                glm::vec4 parentPosition = parentTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec4 childPosition = childTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

                Logger::log("Parent: (" + std::to_string(parentPosition.x) + ", " +
                    std::to_string(parentPosition.y) + ", " +
                    std::to_string(parentPosition.z) + ") Child: (" +
                    std::to_string(childPosition.x) + ", " +
                    std::to_string(childPosition.y) + ", " +
                    std::to_string(childPosition.z) + ")", Logger::INFO);
            }
        }
    }
}