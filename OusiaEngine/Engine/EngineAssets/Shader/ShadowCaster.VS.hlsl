#include "Standard.hlsli"


float3 ApplyShadowBias(float3 positionWS, float3 normalWS, float3 lightDirection)
{
    const float invNdotL = 1.0f - saturate(dot(lightDirection, normalWS));
    const float scale = invNdotL * gShadowBias.y;

    positionWS = lightDirection * gShadowBias.x + positionWS;
    positionWS = normalWS * scale + positionWS;
    return positionWS;
}


float4 main(VSInput input, uint vid : SV_VertexID) : SV_POSITION
{
    float3 worldPosition;
    float3 worldNormal;
    ComputeWorldPositionAndNormal(input, vid, worldPosition, worldNormal);
    
    const float3 lightDirection = (gShadowBias.z != 0.0f) ? normalize(gShadowLightPosition - worldPosition)
                                                          : gShadowLightDirection;
    worldPosition = ApplyShadowBias(worldPosition, worldNormal, lightDirection);
    return mul(float4(worldPosition, 1.0f), gViewProjection);
}

