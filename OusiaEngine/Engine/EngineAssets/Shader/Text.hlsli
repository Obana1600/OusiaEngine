struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint textureIndex : TEXCOORD1;
    nointerpolation float4 texcoordRect : TEXCOORD2;
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
    float4 gOutlineColor;
    float4 gUnderlayColor;
    float gFaceDilate;
    float gOutlineWidth;
    float2 padding;
    float gUnderlayDilate;
    float gUnderlaySoftness;
    float2 gUnderlayUvOffset;
    uint gGlyphBufferIndex;
}
