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
    uint gReceiveShadows;
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


void ComputeWorldPositionAndNormal(VSInput input, uint vid, out float3 worldPosition, out float3 worldNormal)
{
    if (gBoneMatrixBufferIndex != kNoSkinning)
    {
        StructuredBuffer<float4x4> boneMatrices = ResourceDescriptorHeap[gBoneMatrixBufferIndex];
        StructuredBuffer<BoneWeight> boneWeights = ResourceDescriptorHeap[gBoneWeightBufferIndex];
        const BoneWeight boneWeight = boneWeights[vid];

        const float4x4 skinMatrix =
            boneMatrices[boneWeight.boneIndices.x] * boneWeight.weights.x +
            boneMatrices[boneWeight.boneIndices.y] * boneWeight.weights.y +
            boneMatrices[boneWeight.boneIndices.z] * boneWeight.weights.z +
            boneMatrices[boneWeight.boneIndices.w] * boneWeight.weights.w;

        worldPosition = mul(float4(input.position, 1.0f), skinMatrix).xyz;
        worldNormal = normalize(mul(input.normal, (float3x3) skinMatrix));
    }
    else
    {
        worldPosition = mul(float4(input.position, 1.0f), gWorld).xyz;
        worldNormal = normalize(mul(input.normal, (float3x3) gWorldInverseTranspose));
    }
}

#endif // STANDARD_INCLUDED
