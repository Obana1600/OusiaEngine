#include "Sprite.hlsli"


VSOutput main(VSInput input)
{
    VSOutput output;
    
    output.position = mul(input.position, gMat);
    output.texcoord = mul(float4(input.texcoord, 0.0f, 1.0f), gUVTransform).xy;
    output.color = gColor;
    
	return output;
}