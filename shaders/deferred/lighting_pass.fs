#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform int numLights;
uniform Light lights[4];

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    vec3 lighting = Albedo * 0.1; // Ambient term
    vec3 viewDir = normalize(viewPos - FragPos);

    for (int i = 0; i < numLights; ++i) {
        // Diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        float diff = max(dot(Normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].Color;

        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;

        lighting += diffuse + specular;
    }

    FragColor = vec4(lighting, 1.0);
}
