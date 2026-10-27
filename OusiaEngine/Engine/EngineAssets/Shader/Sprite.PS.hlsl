#include "Sprite.hlsli"

SamplerState gLinearWrap : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    return input.color * texture.Sample(gLinearWrap, input.texcoord);
}
