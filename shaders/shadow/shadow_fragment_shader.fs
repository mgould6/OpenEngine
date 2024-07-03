#version 330 core

#define NUM_CASCADES 4

in vec4 FragPosLightSpace[NUM_CASCADES];

uniform sampler2D shadowMap[NUM_CASCADES];
uniform float cascadeSplits[NUM_CASCADES];

float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadowMap)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - 0.005 > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    float shadow = 0.0;
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        if (FragPosLightSpace[i].z <= cascadeSplits[i])
        {
            shadow = ShadowCalculation(FragPosLightSpace[i], shadowMap[i]);
            break;
        }
    }
    gl_FragColor = vec4(vec3(shadow), 1.0);
}
