layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    mat4 u_Projection;
    vec4 u_CameraPosition;
};

struct ObjectData
{
    mat4 Model;
    mat3 NormalMatrix;
};

layout(std430, binding = 2) readonly buffer Objects
{
    ObjectData u_Objects[];
};

vec3 SrgbToLinear(vec3 color)
{
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), greaterThan(color, vec3(0.04045)));
}
