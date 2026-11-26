#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;

#include "include/Common.glsl"

void main()
{
    gl_Position = u_ViewProjection * (u_Model * vec4(a_Position, 1.0));
}

#type fragment
#version 450 core

void main()
{
}
