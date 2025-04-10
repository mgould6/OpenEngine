void main() {
    mat4 boneTransform = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; i++) {
        int boneIndex = aBoneIDs[i];
        float weight = aWeights[i];
        if (boneIndex >= 0 && boneIndex < 100 && weight > 0.0) {
            boneTransform += boneTransforms[boneIndex] * weight;
            totalWeight += weight;
        }
    }

    // Normalize to correct unintentional scale from weighted sum
    if (totalWeight > 0.0)
        boneTransform /= totalWeight;
    else
        boneTransform = mat4(1.0);

    vec4 worldPosition = model * boneTransform * vec4(aPos, 1.0);
    FragPos = vec3(worldPosition);
    Normal = mat3(transpose(inverse(model * boneTransform))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * worldPosition;
}
