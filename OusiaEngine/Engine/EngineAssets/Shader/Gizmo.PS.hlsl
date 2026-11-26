#include "Gizmo.hlsli"


float4 main(VSOutput input) : SV_TARGET
{
    return float4(input.color.rgb, input.color.a * gAlphaScale);
}
