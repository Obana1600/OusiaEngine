#ifndef GLOBAL_SAMPLERS_INCLUDED
#define GLOBAL_SAMPLERS_INCLUDED


SamplerState gLinearWrap : register(s0);
SamplerState gLinearClamp : register(s1);
SamplerState gPointWrap : register(s2);
SamplerState gPointClamp : register(s3);
SamplerComparisonState gLinearClampCompare : register(s4);

#endif // GLOBAL_SAMPLERS_INCLUDED
