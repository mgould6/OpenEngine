#define GLM_ENABLE_EXPERIMENTAL
// Updated Model.cpp
#include "Model.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../common_utils/Logger.h"

Model::Model(const std::string& path) {
    Logger::log("Loading model from path: " + path, Logger::INFO);
    loadModel(path);
    Logger::log("Loaded " + std::to_string(meshes.size()) + " meshes.", Logger::INFO);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::log("ERROR::ASSIMP::" + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
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

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector;

        // Extract position
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Extract normals
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        // Initialize bone weights to zero
        vertex.BoneIDs = glm::ivec4(0);
        vertex.Weights = glm::vec4(0.0f);

        vertices.push_back(vertex);
    }

    // Extract indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    for (auto& vertex : vertices) {
        float weightSum = vertex.Weights[0] + vertex.Weights[1] + vertex.Weights[2] + vertex.Weights[3];
        if (weightSum > 0.0f) {
            vertex.Weights /= weightSum; // Normalize
        }
    }


    // Process bone weights
    if (mesh->mNumBones > 0) {
        Logger::log("Debug: Processing " + std::to_string(mesh->mNumBones) + " bones.", Logger::INFO);
    }

    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = bone->mName.C_Str();

        if (boneMapping.find(boneName) == boneMapping.end()) {
            boneMapping[boneName] = static_cast<int>(bones.size());
            bones.push_back(Bone{ boneName, "" });
        }

        int boneIndex = boneMapping[boneName];
        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            unsigned int vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            for (int k = 0; k < 4; k++) {
                if (vertices[vertexID].Weights[k] == 0.0f) {
                    vertices[vertexID].BoneIDs[k] = boneIndex;
                    vertices[vertexID].Weights[k] = weight;
                    break;
                }
            }
        }
    }

    return Mesh(vertices, indices, textures);
}

void Model::Draw(Shader& shader) {
    // Ensure bone transforms are sent to the shader
    unsigned int boneMatrixLocation = glGetUniformLocation(shader.ID, "boneTransforms");
    if (boneMatrixLocation == -1) {
        Logger::log("Warning: Shader does not have 'boneTransforms' uniform!", Logger::WARNING);
    }
    else {
        std::vector<glm::mat4> boneMatrices;
        for (const auto& bone : bones) {
            glm::mat4 boneTransform = getBoneTransform(bone.name);
            boneMatrices.push_back(boneTransform);

            Logger::log("Debug: Final Bone Transform Before Sending - " + bone.name +
                " | Pos: " + std::to_string(boneTransform[3][0]) + ", " +
                std::to_string(boneTransform[3][1]) + ", " +
                std::to_string(boneTransform[3][2]), Logger::INFO);
        }

        glUniformMatrix4fv(boneMatrixLocation, boneMatrices.size(), GL_FALSE, &boneMatrices[0][0][0]);
        Logger::log("Debug: Bone transforms sent to shader. Count: " + std::to_string(boneMatrices.size()), Logger::INFO);
    }

    // Debug: Log first vertex position to verify data is valid
    if (!meshes.empty() && !meshes[0].vertices.empty()) {
        const Vertex& firstVertex = meshes[0].vertices[0];
        Logger::log("Debug: First Vertex Position: " +
            std::to_string(firstVertex.Position.x) + ", " +
            std::to_string(firstVertex.Position.y) + ", " +
            std::to_string(firstVertex.Position.z), Logger::INFO);

        Logger::log("Debug: First Vertex Bone Weights: " +
            std::to_string(firstVertex.Weights[0]) + ", " +
            std::to_string(firstVertex.Weights[1]) + ", " +
            std::to_string(firstVertex.Weights[2]) + ", " +
            std::to_string(firstVertex.Weights[3]), Logger::INFO);
    }
    else {
        Logger::log("Warning: No vertex data available in meshes!", Logger::WARNING);
    }

    // Draw all meshes
    for (auto& mesh : meshes) {
        mesh.Draw(shader);
    }
}




glm::vec3 Model::getBoundingBoxCenter() const {
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }
    }
    return (min + max) / 2.0f;
}

float Model::getBoundingBoxRadius() const {
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }
    }
    return glm::distance(min, max) / 2.0f;
}

const std::vector<Bone>& Model::getBones() const {
    return bones;
}

const std::unordered_map<std::string, glm::mat4>& Model::getBoneTransforms() const {
    return boneTransforms;
}

void Model::setBoneTransform(const std::string& boneName, const glm::mat4& transform) {
    boneTransforms[boneName] = transform;

    Logger::log("Debug: Bone " + boneName + " Transform Applied: " +
        std::to_string(transform[3][0]) + ", " +
        std::to_string(transform[3][1]) + ", " +
        std::to_string(transform[3][2]), Logger::INFO);
}





const glm::mat4& Model::getBoneTransform(const std::string& boneName) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    auto it = boneTransforms.find(boneName);
    return it != boneTransforms.end() ? it->second : identity;
}