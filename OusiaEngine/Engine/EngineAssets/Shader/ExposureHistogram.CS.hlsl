#include "Exposure.hlsli"


static const uint kGroupSize = 16;

groupshared uint gsBins[kExposureBinCount];


uint ToBin(float3 color)
{
    const float luminance = dot(color, float3(0.2126729f, 0.7151522f, 0.0721750f));
    
    if (luminance < 0.0001f)
    {
        return 0;
    }

    const float t = saturate((log2(luminance) - gHistogramMinLog2) / gHistogramLog2Range);
    return (uint) (t * (kExposureBinCount - 2)) + 1;
}

[numthreads(kGroupSize, kGroupSize, 1)]
void main(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    gsBins[index] = 0;
    GroupMemoryBarrierWithGroupSync();

    if (id.x < gWidth && id.y < gHeight)
    {
        Texture2D<float4> source = ResourceDescriptorHeap[gSourceIndex];
        InterlockedAdd(gsBins[ToBin(source.Load(int3(id.xy, 0)).rgb)], 1);
    }
    GroupMemoryBarrierWithGroupSync();
    
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[gBufferIndex];
    buffer.InterlockedAdd(index * 4, gsBins[index]);
}
