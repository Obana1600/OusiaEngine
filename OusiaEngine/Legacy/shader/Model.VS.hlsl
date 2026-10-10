#include "Model.hlsli"


VSOutput main(VSInput input)
{
    VSOutput output;
    
    float4x4 viewProjection = mul(gView, gProjection);
    output.position = mul(mul(float4(input.position, 1.0f), gWorld), viewProjection);
    output.normal = normalize(mul(input.normal, (float3x3) gWorld));
    output.texcoord = input.texcoord;
    
    return output;
}