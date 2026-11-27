#ifndef STANDARD_INCLUDED
#define STANDARD_INCLUDED

#include "PerCamera.hlsli"
#include "PerFrame.hlsli"


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

static const uint kLightingNone = 0;
static const uint kLightingLambert = 1;
static const uint kLightingHalfLambert = 2;

#endif // STANDARD_INCLUDED
