#pragma once


cbuffer PerCamera : register(b2)
{
    float4x4 gView;
    float4x4 gProjection;
    float4x4 gViewProjection;
    float3 gCameraPosition;
    uint gCameraDepthTextureIndex;
    float4 gCameraScreenSize; // (幅, 高さ, 1 / 幅, 1 / 高さ)
    uint gCameraOpaqueTextureIndex;
    uint3 gCameraPadding;
}

static const uint kNoTexture = 0xFFFFFFFF;


float2 GetNormalizedScreenSpaceUV(float2 positionCS)
{
    return positionCS * gCameraScreenSize.zw;
}
