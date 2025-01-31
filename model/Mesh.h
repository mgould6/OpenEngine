#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include "../shaders/Shader.h"

// Define a texture struct
struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::ivec4 BoneIDs = glm::ivec4(0);  // Bone indices (up to 4 influences per vertex)
    glm::vec4 Weights = glm::vec4(0.0f); // Bone weights (corresponding to BoneIDs)

};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};

#endif
