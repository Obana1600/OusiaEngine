#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct DirectionLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float intensity; // 輝度
};

ConstantBuffer<Material> gMaterial0 : register(b0);
ConstantBuffer<DirectionLight> gDirectionLight : register(b1);
Texture2D<float32_t4> gTexture0 : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutPut input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture0.Sample(gSampler, input.texcoord);
    if (gMaterial0.enableLighting != 0) // Lightingする場合
    {
        float NdotL = dot(normalize(input.normal), -gDirectionLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f); // halfLambert = (0.5(n·l) + 0.5)^2
        output.color = gMaterial0.color * textureColor * gDirectionLight.color * cos * gDirectionLight.intensity;
    }
    else // Lightingしない場合。前回までと同じ演算
    {
        output.color = gMaterial0.color * textureColor;
    }
    return output;
}