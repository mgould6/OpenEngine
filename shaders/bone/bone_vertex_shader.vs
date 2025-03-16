#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransforms[100];  // Ensure correct bone count explicitly

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    mat4 boneTransform = mat4(0.0);
    
    // Explicitly simple testing with the first Bone ID and weight to isolate issue
    int boneIndex = aBoneIDs[0];
    float weight = aWeights[0];

    // Explicitly validate bone index
    if (boneIndex >= 0 && boneIndex < 100) {
        boneTransform = boneTransforms[boneIndex] * weight;
    } else {
        boneTransform = mat4(1.0); // Explicit fallback for invalid bone index
    }

    vec4 worldPosition = model * boneTransform * vec4(aPos, 1.0);
    FragPos = vec3(worldPosition);
    Normal = mat3(transpose(inverse(model * boneTransform))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * worldPosition;
}
