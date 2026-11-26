#pragma once

#include "PerCamera.hlsli"


struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};

cbuffer PerDraw : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInverseTranspose;
    uint gBoneMatrixBufferIndex;
    uint gBoneWeightBufferIndex;
}

static const uint kNoSkinning = 0xFFFFFFFF;

struct BoneWeight
{
    uint4 boneIndices;
    float4 weights;
};

cbuffer PerMaterial : register(b1)
{
    float4x4 gUvTransform;
    float4 gColor;
    uint gTextureIndex;
    uint gLightingMode;
}

static const uint kLightTypeSpot = 0;
static const uint kLightTypeDirectional = 1;
static const uint kLightTypePoint = 2;
static const uint kMaxLights = 16;

static const uint kLightingNone = 0;
static const uint kLightingLambert = 1;
static const uint kLightingHalfLambert = 2;

struct ShaderLight
{
    float3 position;
    uint type;
    float3 direction;
    float range;
    float3 color;
    float spotAngleScale;
    float spotAngleOffset;
    float3 padding;
};

cbuffer PerFrame : register(b3)
{
    uint gLightCount;
    uint3 gPadding;
    ShaderLight gLights[kMaxLights];
}
