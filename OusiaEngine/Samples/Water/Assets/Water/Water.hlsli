#pragma once

// PerCamera.hlsli・GlobalSamplers.hlsli は Declare*.hlsli が読む（ここでも書くと、#pragma once が効かずに二重に読まれる）
#include "DeclareDepthTexture.hlsli"
#include "DeclareOpaqueTexture.hlsli"
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
    float2 texcoord : TEXCOORD0;
};

cbuffer PerDraw : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInverseTranspose;
    uint gBoneMatrixBufferIndex;
    uint gBoneWeightBufferIndex;
}

// Water.shader の "properties" と同じ順
cbuffer WaterProperties : register(b1)
{
    uint _DisplacementMap;
    uint _NormalMap;
    float _WaveStrength; // 0 なら波のテクスチャを読まない（スクリプトが動く前・エディタでは平ら）
    float _RefractionStrength;
    float4 _ShallowColor;
    float4 _DeepColor;
    float4 _SkyColor;
    float4 _FoamColor;
    float _Absorption;
    float _ShoreFoamDistance;
    float _CrestFoam;
    float _Shininess;
}
