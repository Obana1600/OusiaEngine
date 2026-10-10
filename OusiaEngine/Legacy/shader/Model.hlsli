struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};


struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
};


cbuffer WorldTransform : register(b0)
{
    float4x4 gWorld;
}


cbuffer Camera : register(b1)
{
    float4x4 gView;
    float4x4 gProjection;
}


cbuffer Material : register(b2)
{
    float4x4 gUvTransform;
    float3 gColor;
    float gAlpha;
    int gLightMode;
}


struct DirectionLight
{
    float4 color;
    float3 direction;
    float intensity;
};
ConstantBuffer<DirectionLight> gLight : register(b3);