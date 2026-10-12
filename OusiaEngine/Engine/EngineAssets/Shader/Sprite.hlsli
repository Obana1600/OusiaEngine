struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

cbuffer SpriteConstants : register(b0)
{
    float4x4 gWorldViewProj;
    float4x4 gUvTransform;
    float4 gColor;
    float4 gPositionRect; // アンカー用
    float4 gTexcoordRect; // 反転用
    uint gTextureIndex;
}
