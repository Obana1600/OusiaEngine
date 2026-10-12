#include "Blit.hlsli"

VSOutput main( uint vid : SV_VertexID )
{
    VSOutput output;
    output.texcoord = float2((vid << 1) & 2, vid & 2);
    output.position = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}
