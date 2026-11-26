#include "Particle.hlsli"

#include "GlobalSamplers.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    float4 color = input.color * texture.Sample(gLinearClamp, input.texcoord);
    
    // ブレンドモードが乗算時の処理
    if (gBlendMode == kBlendModeMultiply)
    {
        color.rgb = lerp(1.0f, color.rgb, color.a);
    }
    
    return color;
}