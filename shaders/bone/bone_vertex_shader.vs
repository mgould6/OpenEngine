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
out vec2 TexCoords;
out vec3 PosColor;

void main() {
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i) {
        int boneIndex = aBoneIDs[i];
        float weight = aWeights[i];

        if (boneIndex >= 0 && boneIndex < 100 && weight > 0.0) {
            skinMatrix += boneTransforms[boneIndex] * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight == 0.0)
        skinMatrix = mat4(1.0);  // Fallback if vertex is not weighted

    vec4 worldPosition = model * skinMatrix * vec4(aPos, 1.0);
    FragPos = vec3(worldPosition);
    TexCoords = aTexCoords;

    PosColor = fract(FragPos * 0.05);  // Debug visualization of vertex stability

    gl_Position = projection * view * worldPosition;
}
