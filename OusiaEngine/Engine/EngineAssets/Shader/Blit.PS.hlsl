#include "Blit.hlsli"

#include "GlobalSamplers.hlsli"


cbuffer BlitConstants : register(b0)
{
    uint gSourceIndex;
}


float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> source = ResourceDescriptorHeap[gSourceIndex];
    return source.Sample(gLinearClamp, input.texcoord);
}
