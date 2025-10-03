#version 450 core

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    float a = texture(uTexture, vUV).r;
	FragColor = vec4(vColor);
}
