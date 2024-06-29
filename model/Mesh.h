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
