#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs; // Bone indices
layout (location = 4) in vec4 aWeights;  // Bone weights

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransforms[100];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    mat4 boneTransform = boneTransforms[aBoneIDs[0]] * aWeights[0] +
                         boneTransforms[aBoneIDs[1]] * aWeights[1] +
                         boneTransforms[aBoneIDs[2]] * aWeights[2] +
                         boneTransforms[aBoneIDs[3]] * aWeights[3];

    vec4 animatedPosition = boneTransform * vec4(aPos, 1.0); // Corrected placement

    // Debugging: If bone transformation is NaN or zeroed out, force vertex to a debug position
    if (isnan(animatedPosition.x) || isnan(animatedPosition.y) || isnan(animatedPosition.z) ||
        animatedPosition == vec4(0.0, 0.0, 0.0, 1.0)) {
        animatedPosition = vec4(1.0, 1.0, 1.0, 1.0); // Move to debug position
    }

    gl_Position = projection * view * model * animatedPosition;
}
