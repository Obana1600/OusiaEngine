struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint textureIndex : TEXCOORD1;
};

struct GlyphQuad
{
    float4 positionRect;
    float4 texcoordRect;
    uint textureIndex;
    uint3 padding;
};

cbuffer PerText : register(b0)
{
    float4x4 gWorldViewProj;
    float4 gColor;
    uint gGlyphBufferIndex;
}
