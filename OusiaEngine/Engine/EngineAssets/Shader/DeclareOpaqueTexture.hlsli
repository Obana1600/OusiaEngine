#ifndef DECLARE_OPAQUE_TEXTURE_INCLUDED
#define DECLARE_OPAQUE_TEXTURE_INCLUDED

#include "GlobalSamplers.hlsli"
#include "PerCamera.hlsli"


float3 SampleSceneColor(float2 uv)
{
    if (gCameraOpaqueTextureIndex == kNoTexture)
    {
        return 0.0f;
    }

    Texture2D<float4> opaqueTexture = ResourceDescriptorHeap[gCameraOpaqueTextureIndex];
    return opaqueTexture.SampleLevel(gLinearClamp, uv, 0).rgb;
}


float3 LoadSceneColor(uint2 pixel)
{
    if (gCameraOpaqueTextureIndex == kNoTexture)
    {
        return 0.0f;
    }

    Texture2D<float4> opaqueTexture = ResourceDescriptorHeap[gCameraOpaqueTextureIndex];
    return opaqueTexture.Load(int3(pixel, 0)).rgb;
}

#endif // DECLARE_OPAQUE_TEXTURE_INCLUDED
