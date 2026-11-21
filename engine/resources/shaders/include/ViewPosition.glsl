vec3 GetViewPosition(sampler2D depth, mat4 inverseProjection, vec2 uv)
{
    vec4 position = inverseProjection * vec4(uv * 2.0 - 1.0, texture(depth, uv).r, 1.0);
    return position.xyz / position.w;
}
