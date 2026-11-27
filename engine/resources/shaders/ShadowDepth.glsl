#type vertex
#version 450 core
#extension GL_ARB_shader_draw_parameters : require
layout(location = 0) in vec3 a_Position;

#include "include/Common.glsl"

void main()
{
    ObjectData object = u_Objects[gl_BaseInstanceARB + gl_InstanceID];
    gl_Position = u_ViewProjection * (object.Model * vec4(a_Position, 1.0));
}

#type fragment
#version 450 core

void main()
{
}
