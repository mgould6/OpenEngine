#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>

class Model {
public:
    Model(const std::string& path) {
        loadModel(path);
    }
    void Draw(Shader& shader);
    glm::vec3 getBoundingBoxCenter() const;
    float getBoundingBoxRadius() const;



private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};

#endif
