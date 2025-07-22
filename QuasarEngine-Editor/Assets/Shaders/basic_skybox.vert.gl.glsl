#version 330 core

layout(location = 0) in vec3 inPosition;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(inPosition, 1.0);

    gl_Position = pos.xyww;
    TexCoords = inPosition;
}