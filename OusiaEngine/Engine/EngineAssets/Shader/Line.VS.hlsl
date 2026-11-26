#include "Line.hlsli"


static const float kMaxMiterScale = 4.0f;

float3 SideOf(float3 direction, float3 toCamera)
{
    const float3 side = cross(direction, toCamera);
    const float len = length(side);
    return len > 1e-6f ? side / len : float3(0.0f, 0.0f, 0.0f);
}


VSOutput main(uint vid:SV_VertexID)
{
    StructuredBuffer<PointData> points = ResourceDescriptorHeap[gPointBufferIndex];
    
    const uint index = vid / 2;
    const float sideSign = (vid & 1) ? 1.0f : -1.0f;
    const PointData p = points[index];
    
    const uint lastIndex = gPointCount - 1;
    uint prevIndex = index;
    uint nextIndex = index;
    if (index > 0)
    {
        prevIndex = index - 1;
    }
    else if (gLoop != 0)
    {
        prevIndex = lastIndex - 1;
    }
    if (index < lastIndex)
    {
        nextIndex = index + 1;
    }
    else if (gLoop != 0)
    {
        nextIndex = 1;
    }
    
    const float3 toPrev = p.position - points[prevIndex].position;
    const float3 toNext = points[nextIndex].position - p.position;
    const float3 toCamera = normalize(gCameraPosition - p.position);
    
    float3 sidePrev = SideOf(toPrev, toCamera);
    float3 sideNext = SideOf(toNext, toCamera);
    if (dot(sidePrev, sidePrev) == 0.0f)
    {
        sidePrev = sideNext;
    }
    if (dot(sideNext, sideNext) == 0.0f)
    {
        sideNext = sidePrev;
    }
    
    float3 miter = sidePrev + sideNext;
    float scale = 1.0f;
    if (dot(miter, miter) > 1e-6f)
    {
        miter = normalize(miter);
        scale = min(1.0f / max(dot(miter, sideNext), 1e-3f), kMaxMiterScale);
    }
    else
    {
        miter = sidePrev;
    }
    
    const float3 worldPosition = p.position + miter * (p.width * 0.5f * scale * sideSign);
    
    VSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.texcoord = float2(p.texcoord, (vid & 1) ? 0.0f : 1.0f);
    output.color = p.color;
	return output;
}
