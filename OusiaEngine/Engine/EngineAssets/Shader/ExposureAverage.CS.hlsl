#include "Exposure.hlsli"


groupshared float gsPrefix[kExposureBinCount];
groupshared float gsSum[kExposureBinCount];
groupshared float gsWeight[kExposureBinCount];


[numthreads(kExposureBinCount, 1, 1)]
void main(uint index : SV_GroupIndex)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[gBufferIndex];
    
    const float count = (index == 0) ? 0.0 : (float) buffer.Load(index * 4);
    gsPrefix[index] = count;
    GroupMemoryBarrierWithGroupSync();
    
    // 累積和
    [unroll]
    for (uint offset = 1; offset < kExposureBinCount; offset <<= 1)
    {
        const float add = (index >= offset) ? gsPrefix[index - offset] : 0.0;
        GroupMemoryBarrierWithGroupSync();
        gsPrefix[index] += add;
        GroupMemoryBarrierWithGroupSync();
    }
    
    const float total = gsPrefix[kExposureBinCount - 1];
    const float lower = total * gPercentMin * 0.01;
    const float upper = total * gPercentMax * 0.01;
    const float inclusive = gsPrefix[index];
    const float exclusive = inclusive - count;
    const float used = max(0.0, min(inclusive, upper) - max(exclusive, lower));
    
    // 重みづけ
    const float binLog2 = gHistogramMinLog2 + (index - 0.5) / (kExposureBinCount - 2) * gHistogramLog2Range;
    gsSum[index] = used * binLog2;
    gsWeight[index] = used;
    GroupMemoryBarrierWithGroupSync();
    
    // 合計
    [unroll]
    for (uint stride = kExposureBinCount / 2; stride > 0; stride >>= 1)
    {
        if (index < stride)
        {
            gsSum[index] += gsSum[index + stride];
            gsWeight[index] += gsWeight[index + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (index == 0)
    {
        const float previous = asfloat(buffer.Load(kExposureEV100Offset));
        const bool hasPrevious = buffer.Load(kExposureStateOffset) != 0;

        // 目標
        float target = gLimitMin;
        if (gsWeight[0] > 0.0)
        {
            target = gsSum[0] / gsWeight[0] + kLog2LuminanceToEV100;
        }
        else if (hasPrevious)
        {
            target = previous;
        }
        target = clamp(target, gLimitMin, gLimitMax);

        // 目の慣れ
        float ev100 = target;
        if (gAdaptationProgressive != 0 && hasPrevious)
        {
            const float speed = (target > previous) ? gSpeedDarkToLight : gSpeedLightToDark;
            ev100 = previous + (target - previous) * (1.0 - exp2(-gDeltaTime * speed));
        }
        buffer.Store(kExposureEV100Offset, asuint(ev100));
        buffer.Store(kExposureStateOffset, 1);
        
        buffer.Store(kExposureValueOffset, asuint(1.0 / (1.2 * exp2(ev100 - gCompensation))));
    }
    
    buffer.Store(index * 4, 0);
}
