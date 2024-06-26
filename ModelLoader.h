#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <assimp/scene.h>
#include "Model.h"
#include "Mesh.h"
#include "Texture.h"
#include "Vertex.h"  // Include the Vertex header

class ModelLoader {
public:
    static Model LoadModel(const std::string& path);

private:
    static void ProcessNode(aiNode* node, const aiScene* scene, Model& model);
    static Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
    static std::vector<TextureClass> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory);
};
