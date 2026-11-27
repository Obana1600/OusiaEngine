#pragma once


static const uint kExposureBinCount = 256;
static const uint kExposureValueOffset = kExposureBinCount * 4;
static const uint kExposureEV100Offset = kExposureValueOffset + 4;
static const uint kExposureStateOffset = kExposureEV100Offset + 4;

static const float kLog2LuminanceToEV100 = 3.0;

cbuffer ExposurePassConstants : register(b0)
{
    uint gSourceIndex;
    uint gBufferIndex;
    uint gWidth;
    uint gHeight;
    
    float gHistogramMinLog2;
    float gHistogramLog2Range;
    float gPercentMin;
    float gPercentMax;
    
    float gLimitMin;
    float gLimitMax;
    float gCompensation;
    float gDeltaTime;
    
    float gSpeedDarkToLight;
    float gSpeedLightToDark;
    uint gAdaptationProgressive; // 0: Fixed、1: Progressive
    uint gPadding;
}
