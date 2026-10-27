struct VSInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
    float4 uv0 : TEXCOORD0;
    float4 uv1 : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

cbuffer PerCanvasRenderer : register(b0)
{
    float4x4 gWorldViewProj;
    float4 gColor;
    uint gTextureIndex;
}
