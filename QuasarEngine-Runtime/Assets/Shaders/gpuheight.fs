#version 440 core

in float fHeight;
in vec3 fNormal;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uTextureScale;

void main()
{
    

    FragColor = vec4(texture(uTexture, vec2(TexCoord.x * uTextureScale, TexCoord.y * uTextureScale)).rgb, 1.0);
}