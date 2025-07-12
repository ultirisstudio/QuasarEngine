#version 330 core

in VS_OUT
{
	vec2 fTexCoords;
} fs_in;

out vec4 color;

void main()
{
	color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}