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

out vec4 BoneColor;

vec3 idToColor(int id) {
    return vec3(
        ((id & 0x000000FF) >>  0) / 255.0,
        ((id & 0x0000FF00) >>  8) / 255.0,
        ((id & 0x00FF0000) >> 16) / 255.0
    );
}

void main() {
    mat4 boneTransform = mat4(0.0);
    float totalWeight = dot(aWeights, vec4(1.0));
    if (totalWeight > 0.0) {
        for (int i = 0; i < 4; ++i) {
            if (aWeights[i] > 0.0) {
                boneTransform += boneTransforms[aBoneIDs[i]] * (aWeights[i] / totalWeight);
            }
        }
    } else {
        boneTransform = mat4(1.0);
    }

    gl_Position = projection * view * model * boneTransform * vec4(aPos, 1.0);

    // Explicitly output the primary bone ID for visualization as color
    BoneColor = vec4(idToColor(aBoneIDs[0]), 1.0);
}
