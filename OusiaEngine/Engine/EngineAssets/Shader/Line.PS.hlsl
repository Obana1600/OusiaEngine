#include "Line.hlsli"

#include "GlobalSamplers.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    return input.color * texture.Sample(gLinearWrap, input.texcoord);
}
