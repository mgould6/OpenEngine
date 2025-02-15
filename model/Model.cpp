#define GLM_ENABLE_EXPERIMENTAL
#include "Model.h"
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
        path,
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        Logger::log("ERROR::ASSIMP::" + std::string(importer.GetErrorString()), Logger::ERROR);
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    // Convert the root node's transform to glm and invert it
    //glm::mat4 rootTransform = convertAiMatrix(scene->mRootNode->mTransformation);
    //globalInverseTransform = glm::inverse(rootTransform);
    // Temporarily ignore the root transform to test for orientation issues
    globalInverseTransform = glm::mat4(1.0f);

    // Recursively process the scene
    processNode(scene->mRootNode, scene);

    // Update bone hierarchy (parents)
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

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector;

        // Positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Normals
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        // Initialize bone data to zero
        vertex.BoneIDs = glm::ivec4(0);
        vertex.Weights = glm::vec4(0.0f);

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Normalize vertex weights if necessary
    for (auto& vertex : vertices) {
        float weightSum = vertex.Weights[0] + vertex.Weights[1] + vertex.Weights[2] + vertex.Weights[3];
        if (weightSum > 0.0f) {
            vertex.Weights /= weightSum;
        }
    }

    // Process bones (if any)
    if (mesh->mNumBones > 0) {
        Logger::log("Debug: Processing " + std::to_string(mesh->mNumBones) + " bones.", Logger::INFO);
    }

    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = bone->mName.C_Str();

        // If this bone is not already in our mapping, add it and store its offset matrix.
        if (boneMapping.find(boneName) == boneMapping.end()) {
            boneMapping[boneName] = static_cast<int>(bones.size());
            Bone newBone;
            newBone.name = boneName;
            newBone.parentName = "";
            // Convert Assimp's offset matrix to glm::mat4.
            newBone.offsetMatrix = glm::make_mat4(&bone->mOffsetMatrix.a1);
            bones.push_back(newBone);
        }

        int boneIndex = boneMapping[boneName];
        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            unsigned int vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            // Assign up to 4 bone influences per vertex.
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

void Model::Draw(Shader& shader)
{
    Logger::log("DEBUG: Entering Model::Draw()", Logger::INFO);

    if (meshes.empty())
    {
        Logger::log("ERROR: No meshes found to draw!", Logger::ERROR);
        return;
    }

    // Log bounding box for debugging
    glm::vec3 boundingCenter = getBoundingBoxCenter();
    float boundingRadius = getBoundingBoxRadius();
    Logger::log("DEBUG: Model Bounding Box Center: ("
        + std::to_string(boundingCenter.x) + ", "
        + std::to_string(boundingCenter.y) + ", "
        + std::to_string(boundingCenter.z) + ")", Logger::INFO);
    Logger::log("DEBUG: Model Bounding Box Radius: "
        + std::to_string(boundingRadius), Logger::INFO);

    // Set any model-level transform you want (scale, etc.)
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    shader.setMat4("model", modelMatrix);

    // Prepare to upload final bone matrices
    GLint boneMatrixLocation = glGetUniformLocation(shader.ID, "boneTransforms");
    std::vector<glm::mat4> finalMatrices;
    finalMatrices.resize(bones.size(), glm::mat4(1.0f));

    // Combine the bone’s global transform (boneTransforms[boneName])
    // with the bone’s offsetMatrix, so each bone’s final matrix
    // is: globalTransform * offset.
    for (size_t i = 0; i < bones.size(); i++)
    {
        const Bone& bone = bones[i];
        const std::string& boneName = bone.name;

        // The global transform we set in applyToModel
        glm::mat4 globalTransform = getBoneTransform(boneName);

        // Multiply by offset (bind-pose inverse)
        glm::mat4 finalMatrix = globalTransform * bone.offsetMatrix;
        finalMatrices[i] = finalMatrix;

        // Optional debug
        Logger::log("DEBUG: Sending Bone Transform to Shader - " + boneName, Logger::INFO);
        for (int row = 0; row < 4; row++)
        {
            Logger::log(
                std::to_string(finalMatrix[row][0]) + " " +
                std::to_string(finalMatrix[row][1]) + " " +
                std::to_string(finalMatrix[row][2]) + " " +
                std::to_string(finalMatrix[row][3]),
                Logger::INFO
            );
        }
    }

    // Upload to the shader array
    if (!finalMatrices.empty())
    {
        glUniformMatrix4fv(boneMatrixLocation,
            static_cast<GLsizei>(finalMatrices.size()),
            GL_FALSE,
            &finalMatrices[0][0][0]);
    }

    // Finally draw all meshes
    Logger::log("DEBUG: Drawing " + std::to_string(meshes.size()) + " meshes.", Logger::INFO);
    for (auto& mesh : meshes)
    {
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
