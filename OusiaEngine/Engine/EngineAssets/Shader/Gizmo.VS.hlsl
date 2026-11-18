#include "Gizmo.hlsli"

VSOutput main(uint vid : SV_VertexID)
{
    StructuredBuffer<GizmoVertex> vertices = ResourceDescriptorHeap[gVertexBufferIndex];
    const GizmoVertex v = vertices[vid];

    VSOutput output;
    output.position = mul(float4(v.position, 1.0f), gViewProjection);
    output.color = v.color;
    return output;
}
