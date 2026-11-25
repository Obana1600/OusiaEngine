#include "Blit.hlsli"

struct BloomConstants
{
    float3 tint;
    uint textureIndex;
    uint enabled;
    uint3 padding;
};

struct ColorAdjustmentsConstants
{
    float postExposure;
    float contrast;
    float hueShift;
    float saturation;
    float3 colorFilter;
    uint enabled;
};

struct VignetteConstants
{
    float3 color;
    float intensity;
    float2 center;
    float smoothness;
    float roundness;
};

cbuffer UberPostConstants : register(b0)
{
    uint gSourceIndex;
    uint gTonemappingMode; // 0: None, 1: Neutral, 2: ACES
    uint2 gPadding;
    BloomConstants gBloom;
    ColorAdjustmentsConstants gColorAdjustments;
    VignetteConstants gVignette;
}

SamplerState gLinearClamp : register(s1);


// ---- 色の変換 ----

static const float kLogCCut = 0.011361;
static const float kLogCA = 5.555556;
static const float kLogCB = 0.047996;
static const float kLogCC = 0.244161;
static const float kLogCD = 0.386036;
static const float kLogCE = 5.301883;
static const float kLogCF = 0.092819;

static const float kLogCMidGray = 0.4135884;

float3 LinearToLogC(float3 x)
{
    return select(x > kLogCCut, kLogCC * log10(kLogCA * x + kLogCB) + kLogCD, kLogCE * x + kLogCF);
}

float3 LogCToLinear(float3 x)
{
    return select(x > kLogCE * kLogCCut + kLogCF, (pow(10.0, (x - kLogCD) / kLogCC) - kLogCB) / kLogCA, (x - kLogCF) / kLogCE);
}

float3 RgbToHsv(float3 c)
{
    const float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    const float e = 1.0e-4;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HsvToRgb(float3 c)
{
    const float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126729, 0.7151522, 0.0721750));
}


// ---- Vignette ----

float3 ApplyVignette(float3 color, float2 uv)
{
    float2 dist = abs(uv - gVignette.center) * gVignette.intensity;
    dist.x *= gVignette.roundness;
    float factor = pow(saturate(1.0 - dot(dist, dist)), gVignette.smoothness);
    return color * lerp(gVignette.color, 1.0, factor);
}


// ---- ColorAdjustments ----

float3 ApplyColorAdjustments(float3 color)
{
    color *= gColorAdjustments.postExposure;

    float3 colorLog = LinearToLogC(color);
    colorLog = (colorLog - kLogCMidGray) * gColorAdjustments.contrast + kLogCMidGray;
    color = LogCToLinear(colorLog);

    color *= gColorAdjustments.colorFilter;
    
    color = max(0.0, color);

    float3 hsv = RgbToHsv(color);
    hsv.x = frac(hsv.x + gColorAdjustments.hueShift);
    color = HsvToRgb(hsv);

    float luma = Luminance(color);
    color = luma + gColorAdjustments.saturation * (color - luma);
    return color;
}


// ---- Tonemapping ----

float3 NeutralCurve(float3 x, float a, float b, float c, float d, float e, float f)
{
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 NeutralTonemap(float3 x)
{
    const float a = 0.2;
    const float b = 0.29;
    const float c = 0.24;
    const float d = 0.272;
    const float e = 0.02;
    const float f = 0.3;
    const float whiteLevel = 5.3;
    const float whiteClip = 1.0;

    float3 whiteScale = 1.0 / NeutralCurve(whiteLevel, a, b, c, d, e, f);
    x = NeutralCurve(x * whiteScale, a, b, c, d, e, f);
    x *= whiteScale;
    return x / whiteClip;
}

static const float3x3 kAcesInput =
{
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
};

static const float3x3 kAcesOutput =
{
    1.60475, -0.53108, -0.07367,
    -0.10208, 1.10813, -0.00605,
    -0.00327, -0.07276, 1.07602
};

float3 RrtAndOdtFit(float3 v)
{
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

float3 AcesTonemap(float3 color)
{
    color = mul(kAcesInput, color);
    color = RrtAndOdtFit(color);
    color = mul(kAcesOutput, color);
    return saturate(color);
}



float4 main(VSOutput input) : SV_TARGET
{
    Texture2D<float4> source = ResourceDescriptorHeap[gSourceIndex];
    float3 color = source.Load(int3(input.position.xy, 0)).rgb;
    
    // ブルーム
    if (gBloom.enabled != 0)
    {
        Texture2D<float4> bloom = ResourceDescriptorHeap[gBloom.textureIndex];
        color += bloom.SampleLevel(gLinearClamp, input.texcoord, 0).rgb * gBloom.tint;
    }
    
    // ビネット
    if (gVignette.intensity > 0.0)
    {
        color = ApplyVignette(color, float2(input.texcoord.x, 1.0 - input.texcoord.y));
    }

    // 色調補正
    if (gColorAdjustments.enabled != 0)
    {
        color = ApplyColorAdjustments(color);
    }

    // トーンマップ
    if (gTonemappingMode == 1)
    {
        color = NeutralTonemap(color);
    }
    else if (gTonemappingMode == 2)
    {
        color = AcesTonemap(color);
    }
    
    return float4(color, 1.0);
}
