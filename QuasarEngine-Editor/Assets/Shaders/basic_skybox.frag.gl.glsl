#version 330 core

in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    //FragColor = texture(skybox, TexCoords);
	FragColor = vec4(TexCoords * 0.5 + 0.5, 1.0);
}