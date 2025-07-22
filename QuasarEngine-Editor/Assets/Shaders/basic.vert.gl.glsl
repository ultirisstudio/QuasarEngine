#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

out vec2 outTexCoord;
out vec3 outWorldPos;
out vec3 outNormal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main()
{
    outTexCoord = inTexCoord;
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    outNormal = normalMatrix * inNormal;
    outWorldPos = vec3(model * vec4(inPosition, 1.0));
    gl_Position = projection * view * vec4(outWorldPos, 1.0);
}