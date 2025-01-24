#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform sampler2D normalMap;

uniform float shadowBias = 0.005;
uniform int pcfKernelSize = 1;
uniform float lightIntensity;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -pcfKernelSize; x <= pcfKernelSize; ++x)
    {
        for (int y = -pcfKernelSize; y <= pcfKernelSize; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - shadowBias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= pow((pcfKernelSize * 2 + 1), 2);

    if (projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 materialAmbient = vec3(1.0, 0.5, 0.31);
    vec3 materialDiffuse = vec3(1.0, 0.5, 0.31);
    vec3 materialSpecular = vec3(0.5, 0.5, 0.5);
    float materialShininess = 32.0;

    vec3 norm = normalize(Normal);

    // Sample the normal map
    vec3 normalMap = texture(normalMap, FragPos.xy).rgb;
    norm = normalize(normalMap * 2.0 - 1.0);

    vec3 ambient = 0.3 * materialAmbient * lightIntensity;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * materialDiffuse * lightIntensity;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = spec * materialSpecular * lightIntensity;

    float shadow = ShadowCalculation(FragPosLightSpace);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
    FragColor = vec4(lighting, 1.0);
}
