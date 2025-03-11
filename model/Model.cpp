#define GLM_ENABLE_EXPERIMENTAL
#include "Model.h"
#include "../animation/DebugTools.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../common_utils/Logger.h"
#include <glm/gtc/type_ptr.hpp>


// Forward declaration of convertAiMatrix so it can be used in loadModel()
static glm::mat4 convertAiMatrix(const aiMatrix4x4& from);
// Just place this near the top of Model.cpp
static glm::mat4 convertAiMatrix(const aiMatrix4x4& from)
{
    return glm::mat4(
        from.a1, from.b1, from.c1, from.d1,
        from.a2, from.b2, from.c2, from.d2,
        from.a3, from.b3, from.c3, from.d3,
        from.a4, from.b4, from.c4, from.d4
    );
}
Model::Model(const std::string& path) {
    Logger::log("Loading model from path: " + path, Logger::INFO);
    loadModel(path);
    Logger::log("Loaded " + std::to_string(meshes.size()) + " meshes.", Logger::INFO);
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_LimitBoneWeights
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        Logger::log("ERROR::ASSIMP::" + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    // Explicitly setting globalInverseTransform to identity for isolation test
    globalInverseTransform = glm::mat4(1.0f);
    Logger::log("Explicitly set globalInverseTransform to identity for debugging.", Logger::INFO);

    processNode(scene->mRootNode, scene);

    // Explicit bone mapping log
    Logger::log("==== Explicit Bone Mapping BEGIN ====");
    for (const auto& bonePair : boneMapping) {
        Logger::log("Bone Mapping ID[" + std::to_string(bonePair.second) + "] maps to Bone Name[" + bonePair.first + "]", Logger::INFO);
    }
    Logger::log("==== EXPLICIT BONE MAPPING END ====", Logger::INFO);

    updateBoneHierarchy(scene->mRootNode, "");

    Logger::log("Loaded " + std::to_string(meshes.size()) + " meshes.", Logger::INFO);
}



void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.BoneIDs = glm::ivec4(-1);
        vertex.Weights = glm::vec4(0.0f);
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName(bone->mName.C_Str());

        if (boneName.rfind("DEF-", 0) != 0)
            continue;

        int boneIndex = 0;
        if (boneMapping.find(boneName) == boneMapping.end()) {
            boneIndex = bones.size();
            boneMapping[boneName] = boneIndex;

            Bone newBone;
            newBone.name = boneName;

            // Explicit test: override DEF-spine offset matrix
            if (boneName == "DEF-spine") {
                newBone.offsetMatrix = glm::mat4(1.0f); // Explicit identity override
                Logger::log("Explicitly overriding DEF-spine offset matrix to identity.", Logger::INFO);
            }
            else {
                newBone.offsetMatrix = glm::make_mat4(&bone->mOffsetMatrix.a1);
            }

            bones.push_back(newBone);
        }
        else {
            boneIndex = boneMapping[boneName];
        }

        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            unsigned int vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            bool assigned = false;
            for (int k = 0; k < 4; k++) {
                if (vertices[vertexID].Weights[k] == 0.0f) {
                    vertices[vertexID].BoneIDs[k] = boneIndex;
                    vertices[vertexID].Weights[k] = weight;
                    assigned = true;
                    break;
                }
            }
            if (!assigned) {
                Logger::log("WARNING: Vertex " + std::to_string(vertexID) + " exceeded 4 bone influences.", Logger::WARNING);
            }
        }
    }

    // Explicit normalization and fallback
    for (size_t i = 0; i < vertices.size(); ++i) {
        float totalWeight = vertices[i].Weights[0] + vertices[i].Weights[1] +
            vertices[i].Weights[2] + vertices[i].Weights[3];

        if (totalWeight == 0.0f) {
            vertices[i].BoneIDs = glm::ivec4(0);
            vertices[i].Weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            Logger::log("Vertex " + std::to_string(i) + " explicitly assigned fallback bone (0).", Logger::INFO);
        }
        else {
            vertices[i].Weights /= totalWeight;
        }
    }

    Logger::log("Mesh loaded successfully with explicit vertex logging.", Logger::INFO);
    return Mesh(vertices, indices, textures);
}

void Model::Draw(Shader& shader)
{
    std::vector<glm::mat4> finalMatrices(bones.size(), glm::mat4(1.0f));

    for (size_t i = 0; i < bones.size(); i++)
    {
        glm::mat4 globalTransform;

        if (bones[i].name == "DEF-spine") {
            globalTransform = getBoneTransform(bones[i].name);
            Logger::log("Applying actual transform explicitly to [DEF-spine]", Logger::INFO);
        }
        else {
            globalTransform = glm::mat4(1.0f);  // Explicitly isolate all other bones
            Logger::log("Explicit identity applied to bone [" + bones[i].name + "] for testing", Logger::INFO);
        }

        finalMatrices[i] = globalInverseTransform * globalTransform * bones[i].offsetMatrix;

        Logger::log("Final Matrix for Bone [" + bones[i].name + "] ID [" + std::to_string(i) + "]", Logger::INFO);
    }

    GLint boneMatrixLocation = glGetUniformLocation(shader.ID, "boneTransforms");
    if (!finalMatrices.empty()) {
        glUniformMatrix4fv(boneMatrixLocation, finalMatrices.size(), GL_FALSE, &finalMatrices[0][0][0]);
    }

    for (auto& mesh : meshes)
        mesh.Draw(shader);
}



glm::vec3 Model::getBoundingBoxCenter() const {
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }
    }
    glm::vec3 center = (min + max) / 2.0f;

    Logger::log("DEBUG: Bounding Box Center: (" + std::to_string(center.x) + ", " +
        std::to_string(center.y) + ", " + std::to_string(center.z) + ")", Logger::INFO);

    return center;
}


float Model::getBoundingBoxRadius() const {
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }
    }
    float radius = glm::distance(min, max) / 2.0f;

    Logger::log("DEBUG: Bounding Box Radius: " + std::to_string(radius), Logger::INFO);

    return radius;
}


const std::vector<Bone>& Model::getBones() const {
    return bones;
}

const std::unordered_map<std::string, glm::mat4>& Model::getBoneTransforms() const {
    return boneTransforms;
}

void Model::setBoneTransform(const std::string& boneName, const glm::mat4& transform) {
    boneTransforms[boneName] = transform;
    Logger::log("DEBUG: After Storing Bone " + boneName, Logger::INFO);
}




const glm::mat4& Model::getBoneTransform(const std::string& boneName) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    auto it = boneTransforms.find(boneName);
    return it != boneTransforms.end() ? it->second : identity;
}

std::string Model::getBoneParent(const std::string& boneName) const {
    for (const auto& bone : bones) {
        if (bone.name == boneName) {
            return bone.parentName;
        }
    }
    return "";
}

void Model::forceTestBoneTransform() {
    std::string testBone = "DEF-spine";

    if (boneTransforms.find(testBone) != boneTransforms.end()) {
        Logger::log("Debug: Overriding Bone " + testBone + " with test transformation.", Logger::INFO);
        boneTransforms[testBone] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
    }
    else {
        Logger::log("Error: Bone " + testBone + " not found!", Logger::ERROR);
    }
}
int Model::getBoneIndex(const std::string& boneName) const {
    auto it = boneMapping.find(boneName);
    return (it != boneMapping.end()) ? it->second : -1;
}


// Recursively update the bone hierarchy using the scene graph
void Model::updateBoneHierarchy(const aiNode* node, const std::string& parentName) {
    std::string nodeName(node->mName.C_Str());
    // If this node corresponds to a bone we have, update its parent info
    if (boneMapping.find(nodeName) != boneMapping.end()) {
        for (auto& bone : bones) {
            if (bone.name == nodeName) {
                bone.parentName = parentName;
                break;
            }
        }
    }
    // Process all children with this node as their parent
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        updateBoneHierarchy(node->mChildren[i], nodeName);
    }
}
