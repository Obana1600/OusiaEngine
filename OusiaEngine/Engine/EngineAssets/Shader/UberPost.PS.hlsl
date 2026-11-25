#include "Blit.hlsli"

cbuffer UberPostConstants : register(b0)
{
    uint gSourceIndex;
}

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> source = ResourceDescriptorHeap[gSourceIndex];
    float3 color = source.Load(int3(input.position.xy, 0)).rgb;
	return float4(color, 1.0f);
}
