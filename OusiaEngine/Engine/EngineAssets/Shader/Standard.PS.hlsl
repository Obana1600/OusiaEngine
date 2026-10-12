#include "Standard.hlsli"

SamplerState gLinearWrap : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    const float4 baseColor = gColor * texture.Sample(gLinearWrap, input.texcoord);
    
    if (gLightingMode == kLightingNone)
    {
        return baseColor;
    }
    
    const float3 normal = normalize(input.worldNormal);
    const float nDotL = dot(normal, -gLightDirection);
    
    float diffuse;
    if (gLightingMode == kLightingLambert)
    {
        diffuse = saturate(nDotL);
    }
    else
    {
        diffuse = pow(nDotL * 0.5f + 0.5f, 2.0f);
    }
    
    const float3 color = baseColor.rgb * gLightColor.rgb * gLightIntensity * diffuse;
    return float4(color, baseColor.a);
}
