#include "Textured.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    return texture.Sample(gLinearWrap, input.texcoord);
}