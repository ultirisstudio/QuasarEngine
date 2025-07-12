#version 330 core

struct VertexData
{
        vec3 position;
        vec3 normal;
};

struct Material
{
        vec3 ambient_color;
        vec3 diffuse_color;
        vec3 specular_color;
		
		sampler2D diffuse_texture;
		sampler2D specular_texture;
		
		int use_diffuse_texture;
		int use_specular_texture;
		
        float shininess;
};

struct DirLight {
    vec3 direction;
  
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 diffuse;
    vec3 specular;
};

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTextureCoordinates;

out vec4 color;

uniform Material uMaterial;
uniform vec3 uCameraPosition;

uniform int uUseDirLight;
uniform int uUsePointLight;

#define NR_DIR_LIGHTS 4
#define NR_POINT_LIGHTS 4

uniform DirLight dirLights[NR_DIR_LIGHTS];
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objectColor, vec3 objectSpecular)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);
    // combine results
    vec3 diffuse  = light.diffuse  * diff * objectColor;
    vec3 specular = light.specular * spec * objectSpecular;
    return (diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 objectSpecular)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 diffuse  = light.diffuse  * diff * objectColor;
    vec3 specular = light.specular * spec * objectSpecular;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
} 

void main()
{
		VertexData vertex;
        vertex.position = fPosition;
        vertex.normal = fNormal;

		vec3 objectColor;
		vec3 objectSpecular;

		if (uMaterial.use_diffuse_texture == 1)
		{
			objectColor = texture(uMaterial.diffuse_texture, fTextureCoordinates).rgb;
		}
		else
		{
			objectColor = uMaterial.diffuse_color;
		}

		if (uMaterial.use_specular_texture == 1)
		{
			objectSpecular = texture(uMaterial.specular_texture, fTextureCoordinates).rgb;
		}
		else
		{
			objectSpecular = uMaterial.specular_color;
		}

		vec3 norm = normalize(vertex.normal);
		vec3 viewDir = normalize(uCameraPosition - vertex.position);

        float ambientAtteuation = 0.1;
        vec3 ambient  = ambientAtteuation * objectColor;

        vec3 result = ambient;

        for(int i = 0; i < uUseDirLight; i++)
            result += CalcDirLight(dirLights[i], norm, viewDir, objectColor, objectSpecular);

		for(int i = 0; i < uUsePointLight; i++)
            result += CalcPointLight(pointLights[i], norm, vertex.position, viewDir, objectColor, objectSpecular);
		
        color = vec4(result, 1.0f);

        // gamma correct
        float gamma = 2.2;
        color.rgb = pow(color.rgb, vec3(1.0/gamma));
};