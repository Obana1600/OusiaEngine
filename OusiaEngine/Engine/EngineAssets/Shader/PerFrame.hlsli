#ifndef PER_FRAME_INCLUDED
#define PER_FRAME_INCLUDED


static const uint kLightTypeSpot = 0;
static const uint kLightTypeDirectional = 1;
static const uint kLightTypePoint = 2;
static const uint kMaxLights = 16;
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
    float3 padding;
};

cbuffer PerFrame : register(b3)
{
    float4 gTime;
    
    uint gLightCount;
    uint3 gFramePadding;
    ShaderLight gLights[kMaxLights];
    
    float4x4 gWorldToShadow;
    float4 gShadowBias; // x: depth, y: normal
    float3 gShadowLightDirection;
    uint gShadowmapIndex;
    float4 gShadowParams; // x: 濃さ, y: Soft なら 1, z / w: 距離で薄くする scale / bias
    float4 gShadowmapSize; // (1 / 幅, 1 / 高さ, 幅, 高さ)
    uint gShadowLightIndex;
    uint3 gShadowPadding;
}

#endif // PER_FRAME_INCLUDED
