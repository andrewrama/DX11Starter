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
    float3 tangent       : TANGENT;
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
    float3 tangent        : TANGENT;
    float3 worldPosition  : POSITION;
};

struct VertexToPixel_Sky
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

// Constants
// A constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

// Handy to have this as a constant
static const float PI = 3.14159265359f;

// Basic sample and unpack
float3 SampleAndUnpackNormalMap(Texture2D map, SamplerState samp, float2 uv)
{
    return map.Sample(samp, uv).rgb * 2.0f - 1.0f;
}

// Handle converting tangent-space normal map to world space normal
float3 NormalMapping(Texture2D map, SamplerState samp, float2 uv, float3 normal, float3 tangent)
{
	// Grab the normal from the map
    float3 normalFromMap = SampleAndUnpackNormalMap(map, samp, uv);

	// Gather the required vectors for converting the normal
    float3 N = normal;
    float3 T = normalize(tangent - N * dot(tangent, N));
    float3 B = cross(T, N);

	// Create the 3x3 matrix to convert from TANGENT-SPACE normals to WORLD-SPACE normals
    float3x3 TBN = float3x3(T, B, N);

	// Adjust the normal from the map and simply use the results
    return normalize(mul(normalFromMap, TBN));
}

// Helpers

// Lambert diffuse BRDF - Same as the basic lighting diffuse calculation!
// - NOTE: this function assumes the vectors are already NORMALIZED!
float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

// Calculates diffuse amount based on energy conservation
//
// diffuse   - Diffuse amount
// F         - Fresnel result from microfacet BRDF
// metalness - surface metalness amount 
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Function: GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
	// Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
    float VdotH = saturate(dot(v, h));

	// Final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}


// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
	// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

	// Final value
	// Note: Numerator should be NdotV (or NdotL depending on parameters).
	// However, these are also in the BRDF's denominator, so they'll cancel!
	// We're leaving them out here AND in the BRDF function as the
	// dot products can get VERY small and cause rounding errors.
    return 1 / (NdotV * (1 - k) + k);
}

 
// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
    float3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
	// Pass F out of the function for diffuse balance
    F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

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



float3 DirectionalLightPBR(Light light, float3 normal, float roughness, float metalness, float3 surfaceColor, float3 cameraPos, float3 worldPosition, float3 specularColor)
{
    float3 toLight = normalize(-light.Direction);
    float3 toCam = normalize(cameraPos - worldPosition);
    
    // Calculate the light amounts
    float diff = DiffusePBR(normal, toLight);
    float3 F;
    float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specularColor, F);
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 balancedDiff = DiffuseEnergyConserve(diff, F, metalness);
    
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + spec) * light.Intensity * light.Color;

}

float3 PointLightPBR(Light light, float3 normal, float roughness, float metalness, float3 surfaceColor, float3 cameraPos, float3 worldPosition, float3 specularColor)
{
    float3 toLight = normalize(light.Position - worldPosition);
    float3 toCam = normalize(cameraPos - worldPosition);
    
    // Calculate the light amounts
    float diff = DiffusePBR(normal, toLight);
    float3 F;
    float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specularColor, F);
    float attenuation = Attenuate(light, worldPosition);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 balancedDiff = DiffuseEnergyConserve(diff, F, metalness);
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + spec) * light.Intensity * light.Color * attenuation;
}

// Old lighting functions
float3 DirectionalLight(Light light, float3 normal, float roughness, float3 colorTint, float3 cameraPos, float3 worldPosition, float specScale)
{
    float3 dirToLight = normalize(-light.Direction);
     
    // Used for specular calculation
    float3 R = reflect(-dirToLight, normal);
    float3 V = normalize(cameraPos - worldPosition);
    
    float diffuse = Diffuse(normal, R);
    float specular = Specular(normal, R, V, roughness) * specScale;
    
    return light.Color * (colorTint * diffuse + specular) * light.Intensity;
}

float3 PointLight(Light light, float3 normal, float roughness, float3 colorTint, float3 cameraPos, float3 worldPosition, float specScale)
{
    float3 dirToLight = normalize(light.Position - worldPosition);
     
    // Used for specular calculation
    float3 R = reflect(-dirToLight, normal);
    float3 V = normalize(cameraPos - worldPosition);
    
    float attenuation = Attenuate(light, worldPosition);
    float diffuse = Diffuse(normal, R);
    float specular = Specular(normal, R, V, roughness) * specScale;
    
    return light.Color * (colorTint * diffuse + specular) * attenuation * light.Intensity;
}
#endif