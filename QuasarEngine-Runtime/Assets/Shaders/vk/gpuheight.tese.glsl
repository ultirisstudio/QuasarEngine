#version 450 core
layout (quads, fractional_odd_spacing, cw) in;

struct PointLight { vec3 position; vec3 color; float attenuation; float power; };
struct DirLight   { vec3 direction; vec3 color; float power; };
#define NR_POINT_LIGHTS 4
#define NR_DIR_LIGHTS 4

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    int  usePointLight;
    int  useDirLight;
    PointLight pointLights[NR_POINT_LIGHTS];
    DirLight   dirLights[NR_DIR_LIGHTS];
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object {
    mat4  model;

    vec4  albedo;
    float roughness;
    float metallic;
    float ao;
    int   has_albedo_texture;
    int   has_normal_texture;
    int   has_roughness_texture;
    int   has_metallic_texture;
    int   has_ao_texture;

    float heightMult;
    int   uTextureScale;
} object_ubo;

layout(set = 1, binding = 1) uniform sampler2D heightMap;

layout (location = 0) in  vec2 tcTexCoord[];
layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec3 outWorldPos;

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = tcTexCoord[0];
    vec2 t01 = tcTexCoord[1];
    vec2 t10 = tcTexCoord[2];
    vec2 t11 = tcTexCoord[3];

    vec2 t0 = mix(t00, t01, u);
    vec2 t1 = mix(t10, t11, u);
    vec2 texCoord = mix(t0,  t1,  v);

    outTexCoord = texCoord;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec3 uVec = (p01 - p00).xyz;
    vec3 vVec = (p10 - p00).xyz;
    vec3 n    = normalize(cross(vVec, uVec));

    float h = texture(heightMap, texCoord).r * object_ubo.heightMult;

    vec4 p0 = mix(p00, p01, u);
    vec4 p1 = mix(p10, p11, u);
    vec4 p  = mix(p0,  p1,  v) + vec4(n * h, 0.0);

    vec4 worldPos = object_ubo.model * p;
    outWorldPos   = worldPos.xyz;

    gl_Position = global_ubo.projection * global_ubo.view * worldPos;
}
