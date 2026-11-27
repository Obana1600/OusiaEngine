#include "Standard.hlsli"


VSOutput main(VSInput input, uint vid : SV_VertexID)
{
    float3 worldPosition;
    float3 worldNormal;
    ComputeWorldPositionAndNormal(input, vid, worldPosition, worldNormal);

    VSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.worldPosition = worldPosition;
    output.worldNormal = worldNormal;
    output.texcoord = mul(float4(input.texcoord, 0.0f, 1.0f), gUvTransform).xy;
    return output;
}
