#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoords;

out VS_OUT
{
	vec2 fTexCoords;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
	vs_out.fTexCoords = vTexCoords;
	gl_Position = uProjection * uView * uModel * vec4(vPosition, 1.0f);
}