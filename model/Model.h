// Updated Model.h
#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"
#include <memory>
#include "../animation/SkeletonPose.h"   // required include


class SkeletonPose;                   // correct forward declaration


struct Bone {
    std::string name;
    std::string parentName;
    glm::mat4 offsetMatrix = glm::mat4(1.0f); // NEW: store the bone's offset (bind pose) matrix
    glm::mat4 finalTransform = glm::mat4(1.0f); // Add this line

};


class Model {
public:
    Model(const std::string& path);
    void Draw(Shader& shader);
    glm::vec3 getBoundingBoxCenter() const;
    float getBoundingBoxRadius() const;

    const std::vector<Bone>& getBones() const;
    const std::unordered_map<std::string, glm::mat4>& getBoneTransforms() const;
    void setBoneTransform(const std::string& boneName, const glm::mat4& transform);
    int getBoneIndex(const std::string& boneName) const;

    // Added Method
    const glm::mat4& getBoneTransform(const std::string& boneName) const;
    std::string getBoneParent(const std::string& boneName) const;

    void forceTestBoneTransform();
    void updateBoneHierarchy(const aiNode* node, const std::string& parentName);


    glm::mat4 calculateBoneTransform(const std::string& boneName,
    const std::unordered_map<std::string, glm::mat4>& localTransforms,
        std::unordered_map<std::string, glm::mat4>& globalTransforms);
    std::vector<glm::mat4> getFinalBoneMatrices() const;
    std::unordered_map<std::string, glm::mat4> boneLocalBindTransforms;
    glm::mat4 getBoneOffsetMatrix(const std::string& boneName) const;
    glm::mat4 getGlobalInverseTransform() const;

    glm::mat4 getBindPoseGlobalTransform(const std::string& boneName) const;
    bool hasBone(const std::string& name) const;
    glm::mat4 getLocalBindPose(const std::string& boneName) const;

    // Bind-pose offset with SCALE stripped out   (inverse( bindNoScale ))
    glm::mat4 getBoneOffsetMatrixNoScale(const std::string& boneName) const;
    const SkeletonPose* getBindPose() const { return bindPose.get(); }
    const glm::mat4& getRootTransform() const { return rootTransform; }


private:
    std::vector<Mesh> meshes;
    std::string directory;

    std::vector<Bone> bones;
    std::unordered_map<std::string, glm::mat4> boneTransforms;
    std::unordered_map<std::string, int> boneMapping;
    
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // Store the global inverse for the root node
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);


    std::unordered_map<std::string, glm::mat4> boneGlobalBindPose;


	
    void updateBoneTransforms(const aiNode* node, const glm::mat4& parentTransform);
    std::string getBoneName(int index) const;
    std::unique_ptr<SkeletonPose> bindPose;

    glm::mat4 rootTransform = glm::mat4(1.0f);


};

#endif // MODEL_H