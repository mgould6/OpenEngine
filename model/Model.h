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

struct Bone {
    std::string name;
    std::string parentName;
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

    // Added Method
    const glm::mat4& getBoneTransform(const std::string& boneName) const;
    std::string getBoneParent(const std::string& boneName) const;

    void forceTestBoneTransform();


private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Bone> bones;
    std::unordered_map<std::string, glm::mat4> boneTransforms;
    std::unordered_map<std::string, int> boneMapping;

};

#endif // MODEL_H