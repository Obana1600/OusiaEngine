struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer DrawConstants : register(b0)
{
    float4x4 gWorldViewProj;
    uint gTextureIndex;
};

SamplerState gLinearWrap : register(s0);