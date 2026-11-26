#include "Bloom.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> low = ResourceDescriptorHeap[gLowIndex];
	
    float3 highMip = SampleSource(input.texcoord);
    float3 lowMip = low.SampleLevel(gLinearClamp, input.texcoord, 0.0f).rgb;
	return float4(lerp(highMip, lowMip, gScatter), 1.0f);
}
