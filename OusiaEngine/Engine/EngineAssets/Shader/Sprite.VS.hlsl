#include "Sprite.hlsli"

VSOutput main(uint vid : SV_VertexID)
{
    const bool isRight = (vid & 1) != 0;
    const bool isBottom = (vid & 2) != 0;
    
    const float2 position = float2(
        isRight ? gPositionRect.z : gPositionRect.x,
        isBottom ? gPositionRect.w : gPositionRect.y
    );
    const float2 texcoord = float2(
        isRight ? gTexcoordRect.z : gTexcoordRect.x,
        isBottom ? gTexcoordRect.w : gTexcoordRect.y
    );
    
    VSOutput output;
    output.position = mul(float4(position, 0.0f, 1.0f), gWorldViewProj);
    output.texcoord = mul(float4(texcoord, 0.0f, 1.0f), gUvTransform).xy;
    return output;
}
