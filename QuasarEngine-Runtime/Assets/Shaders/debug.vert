#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoordinates;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTextureCoordinates;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
        vec4 worldPosition = uModel * vec4(vPosition, 1.0f);

        fPosition = worldPosition.xyz;
        fNormal = mat3(transpose(inverse(uModel))) * vNormal;
        fTextureCoordinates = vTextureCoordinates;

        gl_Position = uProjection * uView * worldPosition;
};