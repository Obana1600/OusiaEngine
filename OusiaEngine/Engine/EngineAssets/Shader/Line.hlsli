#pragma once

#include "PerCamera.hlsli"


struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PointData
{
    float3 position;
    float width;
    float4 color;
    float texcoord;
    float3 padding;
};

cbuffer PerLineRenderer : register(b0)
{
    uint gPointBufferIndex;
    uint gPointCount;
    uint gTextureIndex;
    uint gLoop;
}
