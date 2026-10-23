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
    Texture2D<float> atlas = ResourceDescriptorHeap[NonUniformResourceIndex(input.textureIndex)];
    
    const float distance = ToDistance(atlas.Sample(gLinearClamp, input.texcoord));
    const float width = fwidth(distance);
    
    const float faceEdge = -gFaceDilate * 0.5f;
    const float outerEdge = faceEdge - gOutlineWidth * 0.5f;
    const float innerEdge = faceEdge + gOutlineWidth * 0.5f;
    
    const float inner = Coverage(distance, innerEdge, width);
    const float outer = Coverage(distance, outerEdge, width);
    
    // Outline
    const float faceAlpha = gColor.a * inner;
    const float outlineAlpha = gOutlineColor.a * (outer - inner);
    const float alpha = faceAlpha + outlineAlpha;
    float4 color = float4((gColor.rgb * faceAlpha + gOutlineColor.rgb * outlineAlpha) / max(alpha, 1e-5f), alpha);
    
    // Underlay
    if (gUnderlayColor.a > 0.0f)
    {
        const float2 uv = clamp(input.texcoord + gUnderlayUvOffset, input.texcoordRect.xy, input.texcoordRect.zw);
        const float underlayDistance = ToDistance(atlas.Sample(gLinearClamp, uv));
        const float underlayEdge = -gUnderlayDilate * 0.5f;
        
        // softness
        const float underlayAlpha = gUnderlayColor.a
            * smoothstep(underlayEdge - width - gUnderlaySoftness, underlayEdge + width, underlayDistance);
        
        const float combinedAlpha = color.a + underlayAlpha * (1.0f - color.a);
        const float3 rgb = color.rgb * color.a + gUnderlayColor.rgb * underlayAlpha * (1.0f - color.a);
        color = float4(rgb / max(combinedAlpha, 1e-5f), combinedAlpha);
    }
    
    return color;

}