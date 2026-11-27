const float Pi = 3.14159265359;

float DistributionGGX(float normalDotHalf, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float denominator = normalDotHalf * normalDotHalf * (roughnessSquared - 1.0) + 1.0;
    return roughnessSquared / (Pi * denominator * denominator);
}

float VisibilitySmithGGX(float normalDotView, float normalDotLight, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float lightPath = normalDotView * sqrt(normalDotLight * normalDotLight * (1.0 - roughnessSquared) + roughnessSquared);
    float viewPath = normalDotLight * sqrt(normalDotView * normalDotView * (1.0 - roughnessSquared) + roughnessSquared);
    return 0.5 / max(lightPath + viewPath, 1e-5);
}

vec3 FresnelSchlick(vec3 reflectance, float cosine)
{
    float complement = 1.0 - cosine;
    float squared = complement * complement;
    return reflectance + (1.0 - reflectance) * (squared * squared * complement);
}

vec3 EnvironmentBRDF(vec3 reflectance, float perceptualRoughness, float normalDotView)
{
    const vec4 first = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 second = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 terms = perceptualRoughness * first + second;
    float bias = min(terms.x * terms.x, exp2(-9.28 * normalDotView)) * terms.x + terms.y;
    vec2 scaleBias = vec2(-1.04, 1.04) * bias + terms.zw;
    return reflectance * scaleBias.x + scaleBias.y;
}

float GetSpecularOcclusion(float normalDotView, float occlusion, float roughness)
{
    return clamp(pow(normalDotView + occlusion, exp2(-16.0 * roughness - 1.0)) - 1.0 + occlusion, 0.0, 1.0);
}
