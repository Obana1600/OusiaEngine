struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldNormal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};

cbuffer PerDraw : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInverseTranspose;
}

cbuffer PerMaterial : register(b1)
{
    float4x4 gUvTransform;
    float4 gColor;
    uint gTextureIndex;
    uint gLightingMode;
}

cbuffer PerCamera : register(b2)
{
    float4x4 gView;
    float4x4 gProjection;
    float4x4 gViewProjection;
    float3 gCameraPosition;
}

cbuffer PerFrame : register(b3)
{
    float4 gLightColor;
    float3 gLightDirection;
    float gLightIntensity;
}

static const uint kLightingNone = 0;
static const uint kLightingLambert = 1;
static const uint kLightingHalfLambert = 2;
