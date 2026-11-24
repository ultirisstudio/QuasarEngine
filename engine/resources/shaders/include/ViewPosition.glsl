// The projections of the engine keep these entries at 0: x and y do not depend on each other or on w
float GetViewDepth(float depth, mat4 projection)
{
    return (projection[3][2] - depth * projection[3][3]) / (depth * projection[2][3] - projection[2][2]);
}

vec3 GetViewPosition(vec2 uv, float viewDepth, mat4 projection)
{
    vec2 ndc = uv * 2.0 - 1.0;
    float w = projection[2][3] * viewDepth + projection[3][3];
    float x = (ndc.x * w - projection[2][0] * viewDepth - projection[3][0]) / projection[0][0];
    float y = (ndc.y * w - projection[2][1] * viewDepth - projection[3][1]) / projection[1][1];
    return vec3(x, y, viewDepth);
}

// The linear depth of the ambient occlusion marks with this value the pixels where nothing was drawn
const float NothingDrawn = uintBitsToFloat(0x7F800000u);

bool IsSameSurface(float depth, float other)
{
    return abs(other - depth) <= 0.05 * abs(depth);
}
