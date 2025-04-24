#define GLM_ENABLE_EXPERIMENTAL
#include "Model.h"
#include "../animation/DebugTools.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../common_utils/Logger.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp> 

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

    globalInverseTransform = glm::inverse(convertAiMatrix(scene->mRootNode->mTransformation));
    Logger::log("Global inverse transform set from root node transformation.", Logger::INFO);
    Logger::log("Global inverse transform matrix: " + glm::to_string(globalInverseTransform), Logger::INFO);

    processNode(scene->mRootNode, scene);

    Logger::log("==== Explicit Bone Mapping BEGIN ====", Logger::INFO);
    for (const auto& bonePair : boneMapping) {
        Logger::log("Bone Mapping ID[" + std::to_string(bonePair.second) + "] maps to Bone Name[" + bonePair.first + "]", Logger::INFO);
    }
    Logger::log("==== EXPLICIT BONE MAPPING END ====", Logger::INFO);

    if (boneMapping.find("DEF-spine") != boneMapping.end()) {
        int spineIndex = boneMapping["DEF-spine"];
        glm::mat4 spineOffsetMatrix = bones[spineIndex].offsetMatrix;
        Logger::log("DEF-spine offset matrix explicitly logged for verification:", Logger::INFO);
        Logger::log(glm::to_string(spineOffsetMatrix), Logger::INFO);
    }
    else {
        Logger::log("DEF-spine bone not found for explicit offset matrix logging.", Logger::ERROR);
    }

    updateBoneHierarchy(scene->mRootNode, "");
    Logger::log("Loaded " + std::to_string(meshes.size()) + " meshes.", Logger::INFO);

    // Recalculate offset matrices using the global bind pose
    std::unordered_map<std::string, glm::mat4> globalBindPoseCache;
    for (auto& bone : bones) {
        auto it = boneLocalBindTransforms.find(bone.name);
        if (it != boneLocalBindTransforms.end()) {
            glm::mat4 globalBindPose = calculateBoneTransform(bone.name, boneLocalBindTransforms, globalBindPoseCache);
            glm::mat4 recalculatedOffset = glm::inverse(globalBindPose);

            Logger::log("Offset matrix from Assimp for bone [" + bone.name + "]:\n" + glm::to_string(bone.offsetMatrix), Logger::WARNING);
            Logger::log("Recalculated offset matrix from bind pose inverse for bone [" + bone.name + "]:\n" + glm::to_string(recalculatedOffset), Logger::WARNING);

            bone.offsetMatrix = recalculatedOffset;
        }
    }
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


// New helper function to retrieve a bone name by its index.
std::string Model::getBoneName(int index) const {
    if (index >= 0 && index < (int)bones.size()) {
        return bones[index].name;
    }
    return "";
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            Logger::log("Vertex[" + std::to_string(i) + "] missing texcoords, set to (0,0)", Logger::WARNING);
        }

        vertex.BoneIDs = glm::ivec4(-1);
        vertex.Weights = glm::vec4(0.0f);
        vertices.push_back(vertex);
    }

    // Process faces
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process bones and assign influences to vertices
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName(bone->mName.C_Str());

        glm::mat4 offsetMatrix = convertAiMatrix(bone->mOffsetMatrix);
        // Add this debug log immediately after converting Assimp offset matrix
        Logger::log("DEBUG: Loaded Assimp Offset Matrix for bone [" + boneName + "]:\n" +
            glm::to_string(offsetMatrix), Logger::WARNING);

        if (boneName.rfind("DEF-", 0) != 0)
            continue;

        int boneIndex = 0;
        if (boneMapping.find(boneName) == boneMapping.end()) {
            boneIndex = bones.size();
            boneMapping[boneName] = boneIndex;

            Bone newBone;
            newBone.name = boneName;
            newBone.offsetMatrix = offsetMatrix;
            bones.push_back(newBone);
            Logger::log("New bone added: " + boneName + " assigned index: " + std::to_string(boneIndex), Logger::INFO);
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

    for (size_t i = 0; i < vertices.size(); i++) {
        float totalWeight = vertices[i].Weights[0] + vertices[i].Weights[1] +
            vertices[i].Weights[2] + vertices[i].Weights[3];
        if (totalWeight == 0.0f) {
            vertices[i].BoneIDs = glm::ivec4(0);
            vertices[i].Weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            Logger::log("Vertex " + std::to_string(i) + " assigned fallback bone (0).", Logger::INFO);
        }
        else {
            vertices[i].Weights /= totalWeight;
        }
        Logger::log("Vertex[" + std::to_string(i) + "] BoneIDs: [" +
            std::to_string(vertices[i].BoneIDs.x) + ", " +
            std::to_string(vertices[i].BoneIDs.y) + ", " +
            std::to_string(vertices[i].BoneIDs.z) + ", " +
            std::to_string(vertices[i].BoneIDs.w) + "] Weights: [" +
            std::to_string(vertices[i].Weights.x) + ", " +
            std::to_string(vertices[i].Weights.y) + ", " +
            std::to_string(vertices[i].Weights.z) + ", " +
            std::to_string(vertices[i].Weights.w) + "]", Logger::DEBUG);
    }

    Logger::log("Mesh loaded successfully with explicit vertex and Assimp offset matrix logging.", Logger::INFO);
    return Mesh(vertices, indices, textures);
}

void Model::Draw(Shader& shader)
{
    std::vector<glm::mat4> finalMatrices(bones.size(), glm::mat4(1.0f));

    for (size_t i = 0; i < bones.size(); i++) {
        glm::mat4 globalTransform = getBoneTransform(bones[i].name);

        // Combine with offset matrix to move from bind pose to animated pose
        bones[i].finalTransform = globalTransform;
        finalMatrices[i] = bones[i].finalTransform;

        // Debug output (optional)
        Logger::log("Bone [" + bones[i].name + "] FINAL TRANSFORM:", Logger::INFO);
        Logger::log(glm::to_string(bones[i].finalTransform), Logger::INFO);

        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(bones[i].finalTransform, scale, rotation, translation, skew, perspective);
        Logger::log("Bone: " + bones[i].name +
            " Scale: " + glm::to_string(scale) +
            " Translation: " + glm::to_string(translation) +
            " Skew: " + glm::to_string(skew), Logger::INFO);

        DebugTools::logDecomposedTransform(bones[i].name, bones[i].finalTransform);
    }

    shader.use();
    shader.setMat4Array("boneTransforms", finalMatrices);

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

const glm::mat4& Model::getBoneTransform(const std::string& boneName) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    auto it = boneTransforms.find(boneName);
    return it != boneTransforms.end() ? it->second : identity;
}


void Model::setBoneTransform(const std::string& boneName, const glm::mat4& transform) {
    boneTransforms[boneName] = transform;
    Logger::log("DEBUG: After Storing Bone " + boneName, Logger::INFO);
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

    //commented out for now
    //std::string testBone = "DEF-spine";

    //if (boneTransforms.find(testBone) != boneTransforms.end()) {
    //    glm::mat4 originalTransform = boneTransforms[testBone];
    //    glm::mat4 incrementalTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));  // Smaller offset
    //    boneTransforms[testBone] = originalTransform * incrementalTranslation;

    //    Logger::log("DEBUG: Incrementally overriding bone [" + testBone + "] with Y+0.5 translation.", Logger::INFO);
    //    Logger::log("DEBUG: Original Transform: " + glm::to_string(originalTransform), Logger::INFO);
    //    Logger::log("DEBUG: New Incremental Transform: " + glm::to_string(boneTransforms[testBone]), Logger::INFO);
    //}
    //else {
    //    Logger::log("ERROR: Bone " + testBone + " not found in boneTransforms.", Logger::ERROR);
    //}
}

int Model::getBoneIndex(const std::string& boneName) const {
    auto it = boneMapping.find(boneName);
    return (it != boneMapping.end()) ? it->second : -1;
}


// Recursively update the bone hierarchy using the scene graph
void Model::updateBoneHierarchy(const aiNode* node, const std::string& parentName) {



    std::string nodeName(node->mName.C_Str());

    bool isDefBone = nodeName.rfind("DEF-", 0) == 0;

    // If this node is a bone, store its local transformation
    if (boneMapping.find(nodeName) != boneMapping.end()) {
        glm::mat4 local = convertAiMatrix(node->mTransformation);
        boneLocalBindTransforms[nodeName] = local;
        Logger::log("Captured local bind transform for bone [" + nodeName + "]: " + glm::to_string(local), Logger::INFO);
    }

    if (nodeName.find("DEF-upper_arm.L") != std::string::npos ||
        nodeName.find("DEF-forearm.L") != std::string::npos ||
        nodeName.find("DEF-hand.L") != std::string::npos ||
        nodeName.find("DEF-shoulder.L") != std::string::npos) {
        Logger::log("ARM CHAIN NODE FOUND: " + nodeName + "  -> Parent: " + parentName, Logger::WARNING);
    }


    Logger::log("Processing node: " + nodeName + " with parent: " + parentName, Logger::INFO);

    // If this node corresponds to a bone, update its parent info
    if (boneMapping.find(nodeName) != boneMapping.end()) {
        for (auto& bone : bones) {
            if (bone.name == nodeName) {
                bone.parentName = parentName;
                Logger::log("Bone [" + bone.name + "] parent set to: " + parentName, Logger::INFO);
                break;
            }
        }
    }
    // Always recurse — children might still be DEF bones
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        std::string currentName(node->mChildren[i]->mName.C_Str());
        std::string thisParent = boneMapping.count(nodeName) ? nodeName : parentName;
        updateBoneHierarchy(node->mChildren[i], thisParent);
    }

}


// This is the key function for recursively accumulating bone transforms
glm::mat4 Model::calculateBoneTransform(const std::string& boneName,
    const std::unordered_map<std::string, glm::mat4>& localTransforms,
    std::unordered_map<std::string, glm::mat4>& globalTransforms) {

    // If already computed, return the cached transform.
    auto cached = globalTransforms.find(boneName);
    if (cached != globalTransforms.end()) {
        Logger::log("Using cached global transform for bone " + boneName, Logger::INFO);
        return cached->second;
    }

    // Retrieve the local transform. Default to identity if not found.
    glm::mat4 localTransform = glm::mat4(1.0f);
    auto it = localTransforms.find(boneName);
    if (it == localTransforms.end()) {
        // Fallback to bind-pose transform if no animation provided
        auto bindIt = boneLocalBindTransforms.find(boneName);
        if (bindIt != boneLocalBindTransforms.end()) {
            localTransform = bindIt->second;
            Logger::log("Using bind-pose transform for bone [" + boneName + "]", Logger::DEBUG);
        }
        else {
            Logger::log("No local or bind transform found for bone [" + boneName + "]; using identity.", Logger::WARNING);
        }
    }
    else {
        localTransform = it->second;
    }

    // Log local transform specifically for arm bones
    if (boneName == "DEF-upper_arm.L" || boneName == "DEF-upper_arm.R") {
        Logger::log("DEBUG: Arm bone [" + boneName + "] local transform: " + glm::to_string(localTransform), Logger::INFO);
    }

    // Get the parent's name and recursively compute its global transform.
    std::string parentName = getBoneParent(boneName);
    glm::mat4 parentTransform = glm::mat4(1.0f);
    if (!parentName.empty()) {
        parentTransform = calculateBoneTransform(parentName, localTransforms, globalTransforms);
        Logger::log("Bone [" + boneName + "] parent's (" + parentName + ") global transform: " + glm::to_string(parentTransform), Logger::INFO);
    }
    else {
        Logger::log("Bone [" + boneName + "] has no parent; using identity for parent transform.", Logger::INFO);
    }

    // Compute final global transform
    glm::mat4 finalTransform = parentTransform * localTransform;

    // Log final global transform specifically for arm bones
    if (boneName == "DEF-upper_arm.L" || boneName == "DEF-upper_arm.R") {
        Logger::log("DEBUG: Arm bone [" + boneName + "] final global transform: " + glm::to_string(finalTransform), Logger::INFO);
    }

    globalTransforms[boneName] = finalTransform;
    Logger::log("Bone [" + boneName + "] final computed transform: " + glm::to_string(finalTransform), Logger::INFO);
    return finalTransform;
}

std::vector<glm::mat4> Model::getFinalBoneMatrices() const {
    std::vector<glm::mat4> matrices;
    matrices.reserve(bones.size());

    for (const auto& bone : bones) {
        matrices.push_back(bone.finalTransform);
    }
    return matrices;
}

glm::mat4 Model::getBoneOffsetMatrix(const std::string& boneName) const {
    int index = getBoneIndex(boneName);
    if (index >= 0 && index < bones.size()) {
        return bones[index].offsetMatrix;
    }
    return glm::mat4(1.0f); // default to identity if bone not found
}

glm::mat4 Model::getGlobalInverseTransform() const {
    return globalInverseTransform;
}


bool Model::hasBone(const std::string& name) const {
    return boneMapping.find(name) != boneMapping.end();
}
