#include "ShaderIncludes.hlsli"

TextureCube SkyCube : register(t0);

SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel_Sky input) : SV_TARGET
{
    return SkyCube.Sample(BasicSampler, input.sampleDir);
}