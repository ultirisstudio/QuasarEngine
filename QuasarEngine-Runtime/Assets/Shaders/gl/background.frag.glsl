#version 450 core

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube environmentMap;

void main()
{
    vec3 envColor = texture(environmentMap, inWorldPos).rgb;

    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0 / 2.2));

    outColor = vec4(envColor, 1.0);
}
