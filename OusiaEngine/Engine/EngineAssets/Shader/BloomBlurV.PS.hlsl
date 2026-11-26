#include "Bloom.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    float texelSize = gSourceTexelSize.y;
    float2 uv = input.texcoord;
	
    float3 c0 = SampleSource(uv - float2(0.0f, texelSize * 3.23076923f));
    float3 c1 = SampleSource(uv - float2(0.0f, texelSize * 1.38461538f));
    float3 c2 = SampleSource(uv);
    float3 c3 = SampleSource(uv + float2(0.0f, texelSize * 1.38461538f));
    float3 c4 = SampleSource(uv + float2(0.0f, texelSize * 3.23076923f));
	
    float3 color = c0 * 0.07027027f + c1 * 0.31621622f + c2 * 0.22702703f +
                   c3 * 0.31621622f + c4 * 0.07027027f;
	return float4(color, 1.0f);
}
