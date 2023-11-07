#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    float3 colorTint;
    float roughness;
    float3 cameraPos;
    float3 ambientColor;
    Light lights[3];
}

Texture2D SurfaceTexture : register(t0);
Texture2D SpecularMap : register(t1);
Texture2D NormalMap : register(t2);

SamplerState BasicSampler : register(s0);

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

    
    float3 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv).rgb;
    surfaceColor = pow(surfaceColor, 2.2f);

    surfaceColor *= colorTint;
    
    float3 finalColor = surfaceColor * ambientColor;
    
    float specScale = SpecularMap.Sample(BasicSampler, input.uv).r;
    
    for (int i = 0; i < 3; i++)
    {
        Light currentLight = lights[i];
        
        switch (currentLight.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                finalColor += DirectionalLight(currentLight, input.normal, roughness, surfaceColor, cameraPos, input.worldPosition, specScale);
                break;
            
            case LIGHT_TYPE_POINT:
                finalColor += PointLight(currentLight, input.normal, roughness, surfaceColor, cameraPos, input.worldPosition, specScale);
                break;
        }
    }

    finalColor = pow(finalColor, 1.0f / 2.2f);
    return float4(finalColor, 1);
}