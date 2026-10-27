#include "Sprite.hlsli"

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), gWorldViewProj);
    output.color = input.color * gColor;
    output.texcoord = input.uv0.xy;
    return output;
}
