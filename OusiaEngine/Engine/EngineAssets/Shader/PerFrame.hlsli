#ifndef PER_FRAME_INCLUDED
#define PER_FRAME_INCLUDED


static const uint kLightTypeSpot = 0;
static const uint kLightTypeDirectional = 1;
static const uint kLightTypePoint = 2;
static const uint kMaxLights = 16;
static const uint kMaxShadowSlices = 32;
static const uint kNoShadowLight = 0xFFFFFFFF;

struct ShaderLight
{
    float3 position;
    uint type;
    float3 direction;
    float range;
    float3 color;
    
    float spotAngleScale;
    float spotAngleOffset;
    
    float shadowStrength;
    uint shadowSoft;
    int shadowSliceIndex; // -1 なら影なし
};

cbuffer PerFrame : register(b3)
{
    float4 gTime;
    
    uint gLightCount;
    uint3 gFramePadding;
    ShaderLight gLights[kMaxLights];
    
    float4x4 gWorldToShadow;
    float4 gShadowBias; // x: depth, y: normal, z: 1 なら Spot / Point
    float3 gShadowLightDirection;
    uint gShadowmapIndex;
    float4 gShadowParams; // x: 濃さ, y: Soft なら 1, z / w: 距離で薄くする scale / bias
    float4 gShadowmapSize; // (1 / 幅, 1 / 高さ, 幅, 高さ)
    uint gShadowLightIndex;

    float3 gShadowLightPosition;
    float4x4 gAdditionalLightsWorldToShadow[kMaxShadowSlices]; // 区画ごと
    float4 gAdditionalShadowmapSize; // (1 / 幅, 1 / 高さ, 幅, 高さ)
    uint gAdditionalShadowmapIndex;
    float2 gAdditionalShadowFadeParams; // 距離で薄くする scale / bias
    uint gAdditionalShadowPadding;
}

#endif // PER_FRAME_INCLUDED
