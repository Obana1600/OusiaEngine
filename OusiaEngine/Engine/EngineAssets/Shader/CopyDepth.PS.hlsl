#include "Blit.hlsli"


cbuffer CopyDepthConstants : register(b0)
{
    uint gSourceIndex;
}


float main(VSOutput input) : SV_TARGET
{
    Texture2D<float> source = ResourceDescriptorHeap[gSourceIndex];
    return source.Load(int3(input.position.xy, 0));
}
