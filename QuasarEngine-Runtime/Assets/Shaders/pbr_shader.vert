#version 450 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoordinates;
layout(location = 3) in float vTextureIndice;

out vec2 fTextureCoordinates;
out vec3 fWorldPos;
out vec3 fNormal;
flat out float fTextureIndice;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main()
{        
        fTextureCoordinates = vTextureCoordinates;
        fWorldPos = vec3(uModel * vec4(vPosition, 1.0));
        fNormal = uNormalMatrix * vNormal;
        fTextureIndice = vTextureIndice;
        
        gl_Position = uProjection * uView * vec4(fWorldPos, 1.0);
};