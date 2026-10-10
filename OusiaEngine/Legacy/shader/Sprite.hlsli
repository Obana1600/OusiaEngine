struct VSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};


struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};


struct PSOutput
{
    float4 color : SV_TARGET0;
};


cbuffer Sprite : register(b0)
{
    float4x4 gMat;
    float4x4 gUVTransform;
    float4 gColor;
}