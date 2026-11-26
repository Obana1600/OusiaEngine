#include "Bloom.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    float3 color = SampleSource(input.texcoord);
	
    color = min(gClampMax, color);
	
    float brightness = max(color.r, max(color.g, color.b));
    float softness = clamp(brightness - gThreshold + gThresholdKnee, 0.0f, 2.0f * gThresholdKnee);
    softness = (softness * softness) / (4.0f * gThresholdKnee + 1e-4f);
    float multiplier = max(brightness - gThreshold, softness) / max(brightness, 1e-4f);
    color *= multiplier;
	
	return float4(max(color, 0.0f), 1.0f);
}