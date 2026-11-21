vec3 SampleTent(sampler2D source, vec2 uv, vec2 offset)
{
    vec3 color = texture(source, uv).rgb * 4.0;
    color += (texture(source, uv + vec2(-offset.x, 0.0)).rgb + texture(source, uv + vec2(offset.x, 0.0)).rgb) * 2.0;
    color += (texture(source, uv + vec2(0.0, -offset.y)).rgb + texture(source, uv + vec2(0.0, offset.y)).rgb) * 2.0;
    color += texture(source, uv - offset).rgb + texture(source, uv + offset).rgb;
    color += texture(source, uv + vec2(-offset.x, offset.y)).rgb + texture(source, uv + vec2(offset.x, -offset.y)).rgb;
    return color / 16.0;
}
