#include "Blit.hlsli"


cbuffer FullScreenConstants : register(b0)
{
    uint gBlitTextureIndex;
    uint3 gFullScreenPadding;
    float4 gScreenSize; // (幅, 高さ, 1 / 幅, 1 / 高さ)
}

SamplerState gLinearClamp : register(s1);
SamplerState gPointClamp : register(s3);


float4 SampleBlitTexture(float2 uv)
{
    Texture2D<float4> blitTexture = ResourceDescriptorHeap[gBlitTextureIndex];
    return blitTexture.SampleLevel(gLinearClamp, uv, 0);
}

float4 LoadBlitTexture(uint2 pixel)
{
    Texture2D<float4> blitTexture = ResourceDescriptorHeap[gBlitTextureIndex];
    return blitTexture.Load(int3(pixel, 0));
}