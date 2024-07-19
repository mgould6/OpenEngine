#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform sampler2D texture_diffuse1;

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct Spotlight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_LIGHTS 4
uniform DirectionalLight dirLights[MAX_LIGHTS];
uniform PointLight pointLights[MAX_LIGHTS];
uniform Spotlight spotlights[MAX_LIGHTS];

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 0.5 : 1.0;

    return shadow;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);

    // Directional lights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        vec3 lightDir = normalize(-dirLights[i].direction);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 ambient = dirLights[i].ambient * vec3(texture(texture_diffuse1, TexCoords));
        vec3 diffuse = dirLights[i].diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
        vec3 specular = dirLights[i].specular * spec * vec3(texture(texture_diffuse1, TexCoords));
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        float shadow = ShadowCalculation(fragPosLightSpace);
        result += ambient + shadow * (diffuse + specular);
    }

    // Point lights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));
        vec3 ambient = pointLights[i].ambient * vec3(texture(texture_diffuse1, TexCoords));
        vec3 diffuse = pointLights[i].diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
        vec3 specular = pointLights[i].specular * spec * vec3(texture(texture_diffuse1, TexCoords));
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        result += ambient + diffuse + specular;
    }

    // Spotlights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        vec3 lightDir = normalize(spotlights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        float distance = length(spotlights[i].position - FragPos);
        float attenuation = 1.0 / (spotlights[i].constant + spotlights[i].linear * distance + spotlights[i].quadratic * (distance * distance));
        float theta = dot(lightDir, normalize(-spotlights[i].direction));
        float epsilon = spotlights[i].cutOff - spotlights[i].outerCutOff;
        float intensity = clamp((theta - spotlights[i].outerCutOff) / epsilon, 0.0, 1.0);
        vec3 ambient = spotlights[i].ambient * vec3(texture(texture_diffuse1, TexCoords));
        vec3 diffuse = spotlights[i].diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
        vec3 specular = spotlights[i].specular * spec * vec3(texture(texture_diffuse1, TexCoords));
        ambient *= attenuation * intensity;
        diffuse *= attenuation * intensity;
        specular *= attenuation * intensity;
        result += ambient + diffuse + specular;
    }

    FragColor = vec4(result, 1.0);
}
