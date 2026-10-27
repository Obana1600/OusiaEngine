#include "Text.hlsli"

SamplerState gLinearClamp : register(s1);

float ToDistance(float sdf)
{
    return (sdf - 0.5f) * 2.0f;
}

float Coverage(float distance, float edge, float width)
{
    return smoothstep(edge - width, edge + width, distance);
}

float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float> atlas = ResourceDescriptorHeap[gTextureIndex];
    
    const float distance = ToDistance(atlas.Sample(gLinearClamp, input.texcoord));
    const float width = fwidth(distance);
    
    const float faceEdge = -_FaceDilate * 0.5f;
    const float outerEdge = faceEdge - _OutlineWidth * 0.5f;
    const float innerEdge = faceEdge + _OutlineWidth * 0.5f;
    
    const float inner = Coverage(distance, innerEdge, width);
    const float outer = Coverage(distance, outerEdge, width);
    
    // Outline
    const float faceAlpha = input.color.a * inner;
    const float outlineAlpha = _OutlineColor.a * (outer - inner);
    const float alpha = faceAlpha + outlineAlpha;
    float4 color = float4((input.color.rgb * faceAlpha + _OutlineColor.rgb * outlineAlpha) / max(alpha, 1e-5f), alpha);
    
    // Underlay
    if (_UnderlayColor.a > 0.0f)
    {
        const float2 uv = clamp(input.texcoord + _UnderlayUvOffset.xy, input.texcoordRect.xy, input.texcoordRect.zw);
        const float underlayDistance = ToDistance(atlas.Sample(gLinearClamp, uv));
        const float underlayEdge = -_UnderlayDilate * 0.5f;
        
        // softness
        const float underlayAlpha = _UnderlayColor.a
            * smoothstep(underlayEdge - width - _UnderlaySoftness, underlayEdge + width, underlayDistance);
        
        const float combinedAlpha = color.a + underlayAlpha * (1.0f - color.a);
        const float3 rgb = color.rgb * color.a + _UnderlayColor.rgb * underlayAlpha * (1.0f - color.a);
        color = float4(rgb / max(combinedAlpha, 1e-5f), combinedAlpha);
    }
    
    return color;

}