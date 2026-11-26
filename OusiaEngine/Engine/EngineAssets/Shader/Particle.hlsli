#pragma once

#include "PerCamera.hlsli"


struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct ParticleData
{
    float3 position;
    float size;
    float4 color;
};

cbuffer PerParticleSystem : register(b0)
{
    float4x4 gWorld;
    uint gParticleBufferIndex;
    uint gTextureIndex;
    uint gBlendMode;
}

static const uint kBlendModeMultiply = 2;