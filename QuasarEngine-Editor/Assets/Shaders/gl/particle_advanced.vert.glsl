#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 vColor;
layout(location = 2) out float vAlphaFactor;

layout(std140, binding = 0) uniform global_uniform_object
{
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    float time;
};

layout(std140, binding = 1) uniform local_uniform_object
{
    float sizeOverLifeExponent;
    float alphaOverLifeExponent;
    float softFade;
    float padding;
};

struct GPUParticle
{
    vec3 position;
    float size;

    vec4 colorStart;
    vec4 colorEnd;

    float age;
    float lifetime;
    float rotation;
    float random;
};

layout(std430, binding = 2) readonly buffer ParticlesBuffer
{
    GPUParticle particles[];
};

void main()
{
    GPUParticle p = particles[gl_InstanceID];

    float t = clamp(p.age / max(p.lifetime, 0.0001), 0.0, 1.0);

    float sizeFactor = pow(t, sizeOverLifeExponent);
    float size = mix(p.size, p.size * 1.0, sizeFactor);

    float alphaFactor = pow(1.0 - t, alphaOverLifeExponent);
    alphaFactor *= smoothstep(0.0, 1.0, t * softFade);

    vAlphaFactor = alphaFactor;

    vColor = mix(p.colorStart, p.colorEnd, t);

    vTexCoord = inTexCoord;

    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

    float cs = cos(p.rotation);
    float sn = sin(p.rotation);

    vec2 quad = inPosition.xy;
    vec2 rotated = vec2(quad.x * cs - quad.y * sn, quad.x * sn + quad.y * cs);

    vec3 worldPos = p.position + camRight * rotated.x * size + camUp * rotated.y * size;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
