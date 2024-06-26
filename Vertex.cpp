// Vertex.cpp
#include "Vertex.h"

// Define any necessary methods or functions related to the Vertex struct if needed

Vertex::Vertex() : Position(0.0f), Normal(0.0f), TexCoords(0.0f), Tangent(0.0f), Bitangent(0.0f) {}

Vertex::Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& tex, const glm::vec3& tan, const glm::vec3& bitan)
    : Position(pos), Normal(norm), TexCoords(tex), Tangent(tan), Bitangent(bitan) {}
