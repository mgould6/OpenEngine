#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransforms[100];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    // Initialize transform to identity (not zero)
    mat4 boneTransform = mat4(1.0);
    
    // Apply weighted transformations
    boneTransform *= boneTransforms[aBoneIDs[0]] * aWeights[0];
    boneTransform += boneTransforms[aBoneIDs[1]] * aWeights[1];
    boneTransform += boneTransforms[aBoneIDs[2]] * aWeights[2];
    boneTransform += boneTransforms[aBoneIDs[3]] * aWeights[3];

    vec4 animatedPosition = boneTransform * vec4(aPos, 1.0);
    FragPos = vec3(model * animatedPosition);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * model * animatedPosition;
}
