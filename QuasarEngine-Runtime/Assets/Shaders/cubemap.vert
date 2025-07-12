#version 330 core

layout(location = 0) in vec3 vPosition;

out vec3 WorldPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    WorldPos = vPosition;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}