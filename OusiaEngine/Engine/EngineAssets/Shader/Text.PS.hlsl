#include "Text.hlsli"

SamplerState gLinearClamp : register(s1);

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float> atlas = ResourceDescriptorHeap[NonUniformResourceIndex(input.textureIndex)];
    
    const float distance = atlas.Sample(gLinearClamp, input.texcoord);
    const float width = fwidth(distance);
    const float alpha = smoothstep(0.5f - width, 0.5f + width, distance);
    
	return float4(gColor.rgb, gColor.a * alpha);
}