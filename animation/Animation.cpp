#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation::Animation(const std::string& filePath)
    : filePath(filePath), loaded(false), duration(0.0f) {
    loadAnimation(filePath);
}

bool Animation::isLoaded() const {
    return loaded;
}

float Animation::getDuration() const {
    return duration;
}

void Animation::apply(float animationTime, Model* model) {
    if (!loaded || keyframes.empty()) {
        Logger::log("Error: No keyframes loaded in animation!", Logger::ERROR);
        return;
    }

    Keyframe kf1, kf2;
    float factor = 0.0f;
    bool keyframeFound = false;

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (animationTime >= keyframes[i].timestamp && animationTime <= keyframes[i + 1].timestamp) {
            kf1 = keyframes[i];
            kf2 = keyframes[i + 1];
            factor = (animationTime - kf1.timestamp) / (kf2.timestamp - kf1.timestamp);
            keyframeFound = true;
            break;
        }
    }

    if (!keyframeFound) {
        Logger::log("Error: No valid keyframe pair found for animation time: " + std::to_string(animationTime), Logger::ERROR);
        return;
    }

    std::unordered_map<std::string, glm::mat4> globalBoneTransforms;

    for (const auto& [boneName, transform1] : kf1.boneTransforms) {
        if (kf2.boneTransforms.find(boneName) == kf2.boneTransforms.end()) {
            Logger::log("Warning: Bone " + boneName + " missing in second keyframe!", Logger::WARNING);
            continue;
        }

        glm::mat4 transform2 = kf2.boneTransforms.at(boneName);
        glm::mat4 interpolatedTransform = interpolateKeyframes(transform1, transform2, factor);

        Logger::log("Debug: Interpolated Transform for Bone " + boneName, Logger::INFO);
        for (int row = 0; row < 4; row++) {
            Logger::log(
                std::to_string(interpolatedTransform[row][0]) + " " +
                std::to_string(interpolatedTransform[row][1]) + " " +
                std::to_string(interpolatedTransform[row][2]) + " " +
                std::to_string(interpolatedTransform[row][3]), Logger::INFO);
        }

        globalBoneTransforms[boneName] = interpolatedTransform;
    }

    if (globalBoneTransforms.empty()) {
        Logger::log("Error: No valid bone transforms computed!", Logger::ERROR);
        return;
    }

    for (const auto& [boneName, transform] : globalBoneTransforms) {
        std::string parentName = model->getBoneParent(boneName);
        if (!parentName.empty() && globalBoneTransforms.find(parentName) != globalBoneTransforms.end()) {
            globalBoneTransforms[boneName] = globalBoneTransforms[parentName] * transform;
        }

        model->setBoneTransform(boneName, globalBoneTransforms[boneName]);

        Logger::log("Debug: Applied Transform to Bone " + boneName + " | Pos: " +
            std::to_string(globalBoneTransforms[boneName][3][0]) + ", " +
            std::to_string(globalBoneTransforms[boneName][3][1]) + ", " +
            std::to_string(globalBoneTransforms[boneName][3][2]), Logger::INFO);
    }
}

void Animation::loadAnimation(const std::string& filePath) {
    Logger::log("Loading animation from: " + filePath, Logger::INFO);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::log("ERROR::ASSIMP::" + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    if (!scene->HasAnimations()) {
        Logger::log("Error: No animations found in file!", Logger::ERROR);
        return;
    }

    Logger::log("Debug: Found " + std::to_string(scene->mNumAnimations) + " animations in file.", Logger::INFO);

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];
        Logger::log("Debug: Animation Name: " + std::string(anim->mName.C_Str()), Logger::INFO);
        Logger::log("Debug: Animation Duration: " + std::to_string(anim->mDuration), Logger::INFO);
        Logger::log("Debug: Animation Ticks Per Second: " + std::to_string(anim->mTicksPerSecond), Logger::INFO);

        for (unsigned int j = 0; j < anim->mNumChannels; j++) {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            std::string boneName = std::string(nodeAnim->mNodeName.C_Str());
            Logger::log("Checking extracted animation keyframes for " + boneName, Logger::INFO);
            Logger::log("Total Position Keys: " + std::to_string(nodeAnim->mNumPositionKeys), Logger::INFO);
            Logger::log("Total Rotation Keys: " + std::to_string(nodeAnim->mNumRotationKeys), Logger::INFO);
            Logger::log("Total Scaling Keys: " + std::to_string(nodeAnim->mNumScalingKeys), Logger::INFO);

            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                aiVector3D pos = nodeAnim->mPositionKeys[k].mValue;
                Logger::log("Raw Position Key " + std::to_string(k) + " | Bone: " + boneName +
                    " | Pos: " + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z), Logger::INFO);
            }

            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++) {
                aiQuaternion rot = nodeAnim->mRotationKeys[k].mValue;
                Logger::log("Raw Rotation Key " + std::to_string(k) + " | Bone: " + boneName +
                    " | Rot: " + std::to_string(rot.x) + ", " + std::to_string(rot.y) + ", " + std::to_string(rot.z) + ", " + std::to_string(rot.w), Logger::INFO);
            }
        }
    }
}

glm::mat4 Animation::interpolateKeyframes(const glm::mat4& transform1, const glm::mat4& transform2, float factor) {
    glm::vec3 pos1, pos2, scale1, scale2;
    glm::quat rot1, rot2;
    glm::vec4 persp;
    glm::vec3 skew;

    glm::decompose(transform1, scale1, rot1, pos1, skew, persp);
    glm::decompose(transform2, scale2, rot2, pos2, skew, persp);

    glm::vec3 interpolatedPos = glm::mix(pos1, pos2, factor);
    glm::quat interpolatedRot = glm::slerp(rot1, rot2, factor);
    glm::vec3 interpolatedScale = glm::mix(scale1, scale2, factor);

    Logger::log("Debug: Interpolating Keyframes", Logger::INFO);
    Logger::log("Debug: Factor: " + std::to_string(factor), Logger::INFO);
    Logger::log("Debug: Start Position: " + std::to_string(pos1.x) + ", " + std::to_string(pos1.y) + ", " + std::to_string(pos1.z), Logger::INFO);
    Logger::log("Debug: End Position: " + std::to_string(pos2.x) + ", " + std::to_string(pos2.y) + ", " + std::to_string(pos2.z), Logger::INFO);
    Logger::log("Debug: Interpolated Position: " + std::to_string(interpolatedPos.x) + ", " + std::to_string(interpolatedPos.y) + ", " + std::to_string(interpolatedPos.z), Logger::INFO);

    Logger::log("Debug: Start Rotation: " + std::to_string(rot1.x) + ", " + std::to_string(rot1.y) + ", " + std::to_string(rot1.z) + ", " + std::to_string(rot1.w), Logger::INFO);
    Logger::log("Debug: End Rotation: " + std::to_string(rot2.x) + ", " + std::to_string(rot2.y) + ", " + std::to_string(rot2.z) + ", " + std::to_string(rot2.w), Logger::INFO);
    Logger::log("Debug: Interpolated Rotation: " + std::to_string(interpolatedRot.x) + ", " + std::to_string(interpolatedRot.y) + ", " + std::to_string(interpolatedRot.z) + ", " + std::to_string(interpolatedRot.w), Logger::INFO);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), interpolatedPos);
    glm::mat4 rotation = glm::mat4_cast(interpolatedRot);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), interpolatedScale);

    return translation * rotation * scale;
}
