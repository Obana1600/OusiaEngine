#include "Water.hlsli"


VSOutput main(VSInput input)
{
    // CS が書いた変位でグリッドを動かす（オブジェクトの空間で足してから、ワールドに移す）
    float3 position = input.position;
    if (_WaveStrength > 0.0)
    {
        Texture2D<float4> displacementMap = ResourceDescriptorHeap[_DisplacementMap];
        position += displacementMap.SampleLevel(gLinearClamp, input.texcoord, 0).xyz * _WaveStrength;
    }

    const float4 worldPosition = mul(float4(position, 1.0), gWorld);

    VSOutput output;
    output.position = mul(worldPosition, gViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.texcoord = input.texcoord;
    return output;
}
