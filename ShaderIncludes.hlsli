#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT	   1
#define LIGHT_TYPE_SPOT        2
#define MAX_SPECULAR_EXPONENT 256.0f

struct Light
{
    int Type;
    float3 Direction;
    float Range;
    float3 Position;
    float Intensity;
    float3 Color;
    float SpotFalloff;
    float3 Padding;
};

struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float3 localPosition : POSITION; // XYZ position
    float3 normal        : NORMAL;
    float2 uv			 : TEXCOORD;
};


struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float3 normal         : NORMAL;
    float2 uv             : TEXCOORD;
    float3 worldPosition  : POSITION;
};

// Helpers
float Diffuse(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

float Specular(float3 normal, float3 R, float3 V, float roughness)
{
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    float spec = pow(saturate(dot(R, V)), specExponent);
    return spec;
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}


float3 DirectionalLight(Light light, float3 normal, float roughness, float3 colorTint, float3 cameraPos, float3 worldPosition)
{
    float3 dirToLight = normalize(-light.Direction);
     
    // Used for specular calculation
    float3 R = reflect(dirToLight, normal);
    float3 V = normalize(cameraPos - worldPosition);
    
    float diffuse = Diffuse(normal, R);
    float specular = Specular(normal, R, V, roughness);
    
    return light.Color * (colorTint * diffuse + specular) * light.Intensity;
}

float3 PointLight(Light light, float3 normal, float roughness, float3 colorTint, float3 cameraPos, float3 worldPosition)
{
    float3 dirToLight = normalize(light.Position - worldPosition);
     
    // Used for specular calculation
    float3 R = reflect(dirToLight, normal);
    float3 V = normalize(cameraPos - worldPosition);
    
    float attenuation = Attenuate(light, worldPosition);
    float diffuse = Diffuse(normal, R);
    float specular = Specular(normal, R, V, roughness);
    
    return light.Color * (colorTint * diffuse + specular) * attenuation * light.Intensity;
}

#endif