#include "Bloom.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
    float texelSize = gSourceTexelSize.x * 2.0f;
    float2 uv = input.texcoord;
    
    float3 c0 = SampleSource(uv - float2(texelSize * 4.0f, 0.0f));
    float3 c1 = SampleSource(uv - float2(texelSize * 3.0f, 0.0f));
    float3 c2 = SampleSource(uv - float2(texelSize * 2.0f, 0.0f));
    float3 c3 = SampleSource(uv - float2(texelSize * 1.0f, 0.0f));
    float3 c4 = SampleSource(uv);
    float3 c5 = SampleSource(uv + float2(texelSize * 1.0f, 0.0f));
    float3 c6 = SampleSource(uv + float2(texelSize * 2.0f, 0.0f));
    float3 c7 = SampleSource(uv + float2(texelSize * 3.0f, 0.0f));
    float3 c8 = SampleSource(uv + float2(texelSize * 4.0f, 0.0f));
	
    float3 color = c0 * 0.01621622f + c1 * 0.05405405f + c2 * 0.12162162f +
                   c3 * 0.19459459f + c4 * 0.22702703f + c5 * 0.19459459f +
                   c6 * 0.12162162f + c7 * 0.05405405f + c8 * 0.01621622f;
    
    return float4(color, 1.0f);
}
