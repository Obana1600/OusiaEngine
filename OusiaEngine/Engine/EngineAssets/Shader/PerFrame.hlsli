#ifndef PER_FRAME_INCLUDED
#define PER_FRAME_INCLUDED


static const uint kLightTypeSpot = 0;
static const uint kLightTypeDirectional = 1;
static const uint kLightTypePoint = 2;
static const uint kMaxLights = 16;

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
}

#endif // PER_FRAME_INCLUDED
