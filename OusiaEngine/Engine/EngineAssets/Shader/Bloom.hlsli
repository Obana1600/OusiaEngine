#pragma once

#include "Blit.hlsli"
#include "GlobalSamplers.hlsli"


cbuffer BloomPassConstants : register(b0)
{
    uint gSourceIndex;
    uint gLowIndex;
    float2 gSourceTexelSize;
    float gScatter;
    float gClampMax;
    float gThreshold;
    float gThresholdKnee;
}


float3 SampleSource(float2 uv)
{
    Texture2D<float4> source = ResourceDescriptorHeap[gSourceIndex];
    return source.SampleLevel(gLinearClamp, uv, 0).rgb;
}
