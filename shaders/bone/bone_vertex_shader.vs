#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 boneTransforms[100];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    mat4 boneTransform = mat4(0.0);
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;

    if (totalWeight > 0.0) {
        boneTransform += boneTransforms[aBoneIDs.x] * (aWeights.x / totalWeight);
        boneTransform += boneTransforms[aBoneIDs.y] * (aWeights.y / totalWeight);
        boneTransform += boneTransforms[aBoneIDs.z] * (aWeights.z / totalWeight);
        boneTransform += boneTransforms[aBoneIDs.w] * (aWeights.w / totalWeight);
    } else {
        boneTransform = mat4(1.0);
    }

    vec4 animatedPosition = boneTransform * vec4(aPos, 1.0);
    FragPos = vec3(animatedPosition);
    Normal = mat3(transpose(inverse(boneTransform))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * animatedPosition;
}
