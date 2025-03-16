#define GLM_ENABLE_EXPERIMENTAL
#include "Mesh.h"
#include <glad/glad.h>
#include "../common_utils/Logger.h"
#include <glm/gtx/string_cast.hpp>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : vertices(vertices), indices(indices), textures(textures) {
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Bone IDs (Integers)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));
    Logger::log("Debug: VAO Bone IDs bound to location 3.", Logger::INFO);


    // Bone Weights
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weights));
    Logger::log("Debug: VAO Bone Weights bound to location 4.", Logger::INFO);


    // Explicit logging for Vertex Attributes verification:
    Logger::log("Explicitly logging vertex attribute configuration:", Logger::INFO);
    Logger::log("Position bound to location 0", Logger::INFO);
    Logger::log("Normal bound to location 1", Logger::INFO);
    Logger::log("TexCoords bound to location 2", Logger::INFO);
    Logger::log("BoneIDs bound to location 3 (int attribute)", Logger::INFO);
    Logger::log("Bone Weights bound to location 4 (GL_FLOAT)", Logger::INFO);

    Logger::log("Vertex data size: " + std::to_string(vertices.size()), Logger::INFO);
    Logger::log("Indices count: " + std::to_string(indices.size()), Logger::INFO);

    // Explicitly verify one vertex completely
    if (!vertices.empty()) {
        const Vertex& v = vertices[0];
        Logger::log("Vertex[0] Position: " + glm::to_string(v.Position), Logger::INFO);
        Logger::log("Vertex[0] Normal: " + glm::to_string(v.Normal), Logger::INFO);
        Logger::log("Vertex[0] TexCoords: " + glm::to_string(v.TexCoords), Logger::INFO);
        Logger::log("Vertex[0] BoneIDs: [" +
            std::to_string(v.BoneIDs.x) + ", " +
            std::to_string(v.BoneIDs.y) + ", " +
            std::to_string(v.BoneIDs.z) + ", " +
            std::to_string(v.BoneIDs.w) + "]", Logger::INFO);
        Logger::log("Vertex[0] Weights: [" +
            std::to_string(v.Weights.x) + ", " +
            std::to_string(v.Weights.y) + ", " +
            std::to_string(v.Weights.z) + ", " +
            std::to_string(v.Weights.w) + "]", Logger::INFO);
    }



    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) {
    Logger::log("Drawing mesh with VAO: " + std::to_string(VAO), Logger::INFO);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
