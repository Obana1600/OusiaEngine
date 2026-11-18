struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

// Gizmos::Vertex と同じ並び
struct GizmoVertex
{
    float3 position;
    float padding;
    float4 color;
};

cbuffer PerGizmo : register(b0)
{
    uint gVertexBufferIndex;
    float gAlphaScale; // 物の裏に隠れた線（Hidden の Pass）は薄くする
    float2 gPadding;
}

cbuffer PerCamera : register(b2)
{
    float4x4 gView;
    float4x4 gProjection;
    float4x4 gViewProjection;
    float3 gCameraPosition;
}
