#include "Textured.hlsli"

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), gWorldViewProj);
    output.texcoord = input.texcoord;
	return output;
}