// depth_vertex_shader.vs
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

out vec4 FragPosLightSpace;

void main()
{
    vec4 fragPos = model * vec4(aPos, 1.0);
    FragPosLightSpace = lightSpaceMatrix * fragPos;
    gl_Position = FragPosLightSpace;
}
