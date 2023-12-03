#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix world;
    matrix worldInverseTranspose;
    matrix view;
    matrix projection;  
    
    matrix lightView;
    matrix lightProjection;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;

	
    // Multiply the three matrices together first
    matrix wvp = mul(projection, mul(view, world));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

    output.uv = input.uv;
    output.normal = normalize(mul((float3x3) worldInverseTranspose, input.normal));
    output.worldPosition = mul(world, float4(input.localPosition, 1)).xyz;
    output.tangent = normalize(mul((float3x3) world, input.tangent));
    
    matrix shadowWVP = mul(lightProjection, mul(lightView, world));
    output.shadowMapPos = mul(shadowWVP, float4(input.localPosition, 1.0f));

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}