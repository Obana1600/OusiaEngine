#include "Particle.hlsli"


VSOutput main(uint vid:SV_VertexID, uint iid:SV_InstanceID)
{
    StructuredBuffer<ParticleData> particles = ResourceDescriptorHeap[gParticleBufferIndex];
    const ParticleData particle = particles[iid];
    
    const float2 corner = float2((vid & 1) ? 0.5f : -0.5f, (vid & 2) ? -0.5f : 0.5f);
    
    const float3 right = float3(gView._11, gView._21, gView._31);
    const float3 up = float3(gView._12, gView._22, gView._32);
    
    const float3 center = mul(float4(particle.position, 1.0f), gWorld).xyz;
    const float3 worldPosition = center + (corner.x * right + corner.y * up) * particle.size;
    
    VSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.texcoord = float2(corner.x + 0.5f, 0.5f - corner.y);
    output.color = particle.color;
	return output;
}
