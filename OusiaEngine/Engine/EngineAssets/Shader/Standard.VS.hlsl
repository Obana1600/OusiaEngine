#include "Standard.hlsli"


VSOutput main(VSInput input, uint vid : SV_VertexID)
{
    float4 worldPosition;
    float3 worldNormal;
    
    if (gBoneMatrixBufferIndex != kNoSkinning)
    {
        StructuredBuffer<float4x4> boneMatrices = ResourceDescriptorHeap[gBoneMatrixBufferIndex];
        StructuredBuffer<BoneWeight> boneWeights = ResourceDescriptorHeap[gBoneWeightBufferIndex];
        const BoneWeight boneWeight = boneWeights[vid];
        
        const float4x4 skinMatrix =
            boneMatrices[boneWeight.boneIndices.x] * boneWeight.weights.x +
            boneMatrices[boneWeight.boneIndices.y] * boneWeight.weights.y +
            boneMatrices[boneWeight.boneIndices.z] * boneWeight.weights.z +
            boneMatrices[boneWeight.boneIndices.w] * boneWeight.weights.w;
        
        worldPosition = mul(float4(input.position, 1.0f), skinMatrix);
        worldNormal = normalize(mul(input.normal, (float3x3) skinMatrix));
    }
    else
    {
        worldPosition = mul(float4(input.position, 1.0f), gWorld);
        worldNormal = normalize(mul(input.normal, (float3x3) gWorldInverseTranspose));
    }
    
    VSOutput output;
    output.position = mul(worldPosition, gViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = worldNormal;
    output.texcoord = mul(float4(input.texcoord, 0.0f, 1.0f), gUvTransform).xy;
    return output;
}
