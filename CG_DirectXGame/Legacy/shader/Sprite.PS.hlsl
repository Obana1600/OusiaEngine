#include "Sprite.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


PSOutput main(VSOutput input)
{
    PSOutput output;
    
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = input.color * texColor;
    
	return output;
}