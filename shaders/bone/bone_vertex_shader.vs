#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransforms[100];

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
        skinMatrix = mat4(1.0);

    vec4 worldPosition = model * skinMatrix * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPosition;
}
