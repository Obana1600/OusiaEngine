#include "Standard.hlsli"

#include "GlobalSamplers.hlsli"
#include "Shadows.hlsli"


float Diffuse(float3 normal, float3 toLight)
{
    const float nDotL = dot(normal, toLight);
    if (gLightingMode == kLightingLambert)
    {
        return saturate(nDotL);
    }
    return pow(nDotL * 0.5f + 0.5f, 2.0f);
}

float DistanceAttenuation(float distanceSqr, float range)
{
    const float factor = distanceSqr / (range * range);
    float smoothFactor = saturate(1.0f - factor * factor);
    return smoothFactor * smoothFactor / distanceSqr;
}

float AngleAttenuation(ShaderLight light, float3 toLight)
{
    const float sDotL = dot(light.direction, -toLight);
    const float attenuation = saturate(sDotL * light.spotAngleScale + light.spotAngleOffset);
    return attenuation * attenuation;
}


float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> texture = ResourceDescriptorHeap[gTextureIndex];
    const float4 baseColor = gColor * texture.Sample(gLinearWrap, input.texcoord);
    
    if (gLightingMode == kLightingNone)
    {
        return baseColor;
    }
    
    const float3 normal = normalize(input.worldNormal);
    
    float3 lighting = 0.0f;
    for (uint i = 0; i < gLightCount; i++)
    {
        const ShaderLight light = gLights[i];
        
        // 平行光源
        if (light.type == kLightTypeDirectional)
        {
            float shadow = 1.0f;
            if (i == gShadowLightIndex && gReceiveShadows != 0)
            {
                shadow = MainLightShadow(input.worldPosition);
            }
            lighting += light.color * Diffuse(normal, -light.direction) * shadow;
            continue;
        }
        
        const float3 offset = light.position - input.worldPosition;
        const float distanceSqr = max(dot(offset, offset), 0.0001f);
        const float3 toLight = offset * rsqrt(distanceSqr);
        
        float attenuation = DistanceAttenuation(distanceSqr, light.range);
        if (light.type == kLightTypeSpot)
        {
            attenuation *= AngleAttenuation(light, toLight);
        }

        lighting += light.color * Diffuse(normal, toLight) * attenuation;
    }
    
    return float4(baseColor.rgb * lighting, baseColor.a);
}
