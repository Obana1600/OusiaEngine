#ifndef DECLARE_DEPTH_TEXTURE_INCLUDED
#define DECLARE_DEPTH_TEXTURE_INCLUDED

#include "PerCamera.hlsli"


float LoadSceneDepth(uint2 pixel)
{
    if (gCameraDepthTextureIndex == kNoTexture)
    {
        return 1.0f;
    }
    
    Texture2D<float> depthTexture = ResourceDescriptorHeap[gCameraDepthTextureIndex];
    return depthTexture.Load(int3(pixel, 0));
}


float SampleSceneDepth(float2 uv)
{
    if (gCameraDepthTextureIndex == kNoTexture)
    {
        return 1.0f;
    }
    
    Texture2D<float> depthTexture = ResourceDescriptorHeap[gCameraDepthTextureIndex];
    uint width, height;
    depthTexture.GetDimensions(width, height);

    const uint2 pixel = min(uint2(uv * float2(width, height)), uint2(width - 1, height - 1));
    return depthTexture.Load(int3(pixel, 0));
}


float LinearEyeDepth(float depth)
{
    if (gProjection._m33 == 0.0f)
    {
        return gProjection._m32 / (depth - gProjection._m22);
    }
    return (depth - gProjection._m32) / gProjection._m22;
}

#endif // DECLARE_DEPTH_TEXTURE_INCLUDED
