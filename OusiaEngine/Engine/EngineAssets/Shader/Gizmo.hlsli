#pragma once

#include "PerCamera.hlsli"


struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

struct GizmoVertex
{
    float3 position;
    float padding;
    float4 color;
};

cbuffer PerGizmo : register(b0)
{
    uint gVertexBufferIndex;
    float gAlphaScale;
    float2 gPadding;
}
