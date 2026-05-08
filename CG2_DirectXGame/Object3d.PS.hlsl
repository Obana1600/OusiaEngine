#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

ConstantBuffer<Material> gMaterial0 : register(b0);
Texture2D<float32_t4> gTexture0 : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutPut input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture0.Sample(gSampler, input.texcoord);
    output.color = gMaterial0.color * textureColor;
    return output;
}