#ifndef SHADOWS_INCLUDED
#define SHADOWS_INCLUDED

#include "GlobalSamplers.hlsli"
#include "PerCamera.hlsli"
#include "PerFrame.hlsli"


float SampleShadow_GetTriangleTexelArea(float triangleHeight)
{
    return triangleHeight - 0.5f;
}

void SampleShadow_GetTexelAreas_Tent_3x3(float offset, out float4 computedArea, out float4 computedAreaUncut)
{
    // 両端
    const float offset01SquaredHalved = (offset + 0.5f) * (offset + 0.5f) * 0.5f;
    computedAreaUncut.x = computedArea.x = offset01SquaredHalved - offset;
    computedAreaUncut.w = computedArea.w = offset01SquaredHalved;

    // 内側の 2 つ
    computedAreaUncut.y = SampleShadow_GetTriangleTexelArea(1.5f - offset);
    const float clampedOffsetLeft = min(offset, 0.0f);
    computedArea.y = computedAreaUncut.y - clampedOffsetLeft * clampedOffsetLeft;

    computedAreaUncut.z = SampleShadow_GetTriangleTexelArea(1.5f + offset);
    const float clampedOffsetRight = max(offset, 0.0f);
    computedArea.z = computedAreaUncut.z - clampedOffsetRight * clampedOffsetRight;
}

void SampleShadow_GetTexelWeights_Tent_5x5(float offset, out float3 texelsWeightsA, out float3 texelsWeightsB)
{
    float4 computedArea;
    float4 computedAreaUncut;
    SampleShadow_GetTexelAreas_Tent_3x3(offset, computedArea, computedAreaUncut);

    // 0.16 = テントの面積（2.5 * 2.5）の逆数
    texelsWeightsA.x = 0.16f * computedArea.x;
    texelsWeightsA.y = 0.16f * computedAreaUncut.y;
    texelsWeightsA.z = 0.16f * (computedArea.y + 1.0f);
    texelsWeightsB.x = 0.16f * (computedArea.z + 1.0f);
    texelsWeightsB.y = 0.16f * computedAreaUncut.z;
    texelsWeightsB.z = 0.16f * computedArea.w;
}

float SampleShadow_Tent_5x5(Texture2D<float> shadowmap, float3 shadowCoord)
{
    const float2 tentCenterInTexelSpace = shadowCoord.xy * gShadowmapSize.zw;
    const float2 centerOfFetchesInTexelSpace = floor(tentCenterInTexelSpace + 0.5f);
    const float2 offsetFromTentCenterToCenterOfFetches = tentCenterInTexelSpace - centerOfFetchesInTexelSpace;

    float3 texelsWeightsU_A, texelsWeightsU_B;
    float3 texelsWeightsV_A, texelsWeightsV_B;
    SampleShadow_GetTexelWeights_Tent_5x5(offsetFromTentCenterToCenterOfFetches.x, texelsWeightsU_A, texelsWeightsU_B);
    SampleShadow_GetTexelWeights_Tent_5x5(offsetFromTentCenterToCenterOfFetches.y, texelsWeightsV_A, texelsWeightsV_B);
    
    const float3 fetchesWeightsU = float3(texelsWeightsU_A.xz, texelsWeightsU_B.y) + float3(texelsWeightsU_A.y, texelsWeightsU_B.xz);
    const float3 fetchesWeightsV = float3(texelsWeightsV_A.xz, texelsWeightsV_B.y) + float3(texelsWeightsV_A.y, texelsWeightsV_B.xz);

    // 補間の割合が重みの比になるように、読む位置をずらす
    const float3 fetchesOffsetsU = (float3(texelsWeightsU_A.y, texelsWeightsU_B.xz) / fetchesWeightsU + float3(-2.5f, -0.5f, 1.5f)) * gShadowmapSize.x;
    const float3 fetchesOffsetsV = (float3(texelsWeightsV_A.y, texelsWeightsV_B.xz) / fetchesWeightsV + float3(-2.5f, -0.5f, 1.5f)) * gShadowmapSize.y;
    const float2 bilinearFetchOrigin = centerOfFetchesInTexelSpace * gShadowmapSize.xy;

    float attenuation = 0.0f;
    [unroll]
    for (uint y = 0; y < 3; y++)
    {
        [unroll]
        for (uint x = 0; x < 3; x++)
        {
            const float2 uv = bilinearFetchOrigin + float2(fetchesOffsetsU[x], fetchesOffsetsV[y]);
            attenuation += fetchesWeightsU[x] * fetchesWeightsV[y] *
                shadowmap.SampleCmpLevelZero(gLinearClampCompare, uv, shadowCoord.z);
        }
    }
    return attenuation;
}


float MainLightRealtimeShadow(float3 positionWS)
{
    const float3 shadowCoord = mul(float4(positionWS, 1.0f), gWorldToShadow).xyz;
    
    if (shadowCoord.z <= 0.0f || shadowCoord.z >= 1.0f ||
        any(shadowCoord.xy != saturate(shadowCoord.xy)))
    {
        return 1.0f;
    }

    Texture2D<float> shadowmap = ResourceDescriptorHeap[gShadowmapIndex];
    float attenuation;
    if (gShadowParams.y != 0.0f)
    {
        attenuation = SampleShadow_Tent_5x5(shadowmap, shadowCoord);
    }
    else
    {
        attenuation = shadowmap.SampleCmpLevelZero(gLinearClampCompare, shadowCoord.xy, shadowCoord.z);
    }
    return lerp(1.0f, attenuation, gShadowParams.x);
}

float GetMainLightShadowFade(float3 positionWS)
{
    const float3 cameraToPixel = positionWS - gCameraPosition;
    const float distanceSq = dot(cameraToPixel, cameraToPixel);
    return saturate(distanceSq * gShadowParams.z + gShadowParams.w);
}

float MainLightShadow(float3 positionWS)
{
    const float realtimeShadow = MainLightRealtimeShadow(positionWS);
    return lerp(realtimeShadow, 1.0f, GetMainLightShadowFade(positionWS));
}

#endif // SHADOWS_INCLUDED
