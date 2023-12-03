#include "ShaderIncludes.hlsli"

#define MAX_LIGHTS 64

cbuffer ExternalData : register(b0)
{
    float3 cameraPos;
    int lightNum;
    Light lights[MAX_LIGHTS];  
}

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
Texture2D ShadowMap : register(t4);

SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - N * dot(T, N)); // Gram-Schmidt assumes T&N are normalized!
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
    unpackedNormal = normalize(unpackedNormal); // Don’t forget to normalize!
    
    input.normal = mul(unpackedNormal, TBN);

    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    float3 surfaceColor = Albedo.Sample(BasicSampler, input.uv).rgb;
    surfaceColor = pow(surfaceColor, 2.2f);
    
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    // Perform the perspective divide (divide by W) ourselves
    input.shadowMapPos /= input.shadowMapPos.w;
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    float distToLight = input.shadowMapPos.z;

    
    float shadowAmount = ShadowMap.SampleCmpLevelZero(
        ShadowSampler,
        shadowUV,
        distToLight).r;
    
    float3 finalColor = float3(0, 0, 0);
    
    for (int i = 0; i < lightNum; i++)
    {
        Light currentLight = lights[i];
        
        switch (currentLight.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                float3 lightResult = DirectionalLightPBR(currentLight, input.normal, roughness, metalness, surfaceColor, cameraPos, input.worldPosition, specularColor);
                if (i == 0)
                {
                    lightResult *= shadowAmount;
                }
                finalColor += lightResult;
                break;
            
            case LIGHT_TYPE_POINT:
                finalColor += PointLightPBR(currentLight, input.normal, roughness, metalness, surfaceColor, cameraPos, input.worldPosition, specularColor);
                break;
        }
    }

    finalColor = pow(finalColor, 1.0f / 2.2f);
    return float4(finalColor, 1);
}