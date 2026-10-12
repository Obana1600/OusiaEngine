#include "Standard.hlsli"

VSOutput main(VSInput input)
{
    const float4 worldPosition = mul(float4(input.position, 1.0f), gWorld);
    
    VSOutput output;
    output.position = mul(worldPosition, gViewProjection);
    output.worldNormal = normalize(mul(input.normal, (float3x3) gWorldInverseTranspose));
    output.texcoord = mul(float4(input.texcoord, 0.0f, 1.0f), gUvTransform).xy;
	return output;
}
