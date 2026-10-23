#include "Text.hlsli"

VSOutput main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    StructuredBuffer<GlyphQuad> quads = ResourceDescriptorHeap[gGlyphBufferIndex];
    const GlyphQuad quad = quads[iid];
    
    const bool isRight = (vid & 1) != 0;
    const bool isBottom = (vid & 2) != 0;
    
    const float2 position = float2(
        isRight ? quad.positionRect.z : quad.positionRect.x,
        isBottom ? quad.positionRect.w : quad.positionRect.y
    );
    const float2 texcoord = float2(
        isRight ? quad.texcoordRect.z : quad.texcoordRect.x,
        isBottom ? quad.texcoordRect.w : quad.texcoordRect.y
    );
    
    VSOutput output;
    output.position = mul(float4(position, 0.0f, 1.0f), gWorldViewProj);
    output.texcoord = texcoord;
    output.textureIndex = quad.textureIndex;
	return output;
}
