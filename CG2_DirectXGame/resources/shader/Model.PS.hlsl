#include "Model.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gUvTransform).xy;
    float4 texColor = gTexture.Sample(gSampler, uv);
    
    float3 baseColor = gColor * texColor.rgb;
    
    float3 rgb;
    if (gLightMode == 0)
    {
        rgb = baseColor;
    }
    else
    {
        float NdotL = dot(normalize(input.normal), -normalize(gLight.direction));
        float cos = (gLightMode == 1)
            ? saturate(NdotL)
            : pow(NdotL * 0.5f + 0.5f, 2.0f);
    
        rgb = baseColor * gLight.color.rgb * cos * gLight.intensity;
    }
       
    return float4(rgb, gAlpha * texColor.a);
}