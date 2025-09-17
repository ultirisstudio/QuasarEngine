#version 450 core

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;

out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
