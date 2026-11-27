#include "Water.hlsli"


// 最初の平行光源（なければ真上から白）
void FindMainLight(out float3 direction, out float3 color)
{
    direction = float3(0.0, -1.0, 0.0);
    color = 1.0;
    for (uint i = 0; i < gLightCount; i++)
    {
        if (gLights[i].type == kLightTypeDirectional)
        {
            direction = gLights[i].direction;
            color = gLights[i].color;
            return;
        }
    }
}


float4 main(VSOutput input) : SV_TARGET
{
    // 法線: CS が書いたもの + 時間で流れる細かいさざ波
    float3 normal = float3(0.0, 1.0, 0.0);
    float crest = 1.0;
    if (_WaveStrength > 0.0)
    {
        Texture2D<float4> normalMap = ResourceDescriptorHeap[_NormalMap];
        const float4 sampled = normalMap.SampleLevel(gLinearClamp, input.texcoord, 0);
        normal = normalize(mul(lerp(float3(0.0, 1.0, 0.0), sampled.xyz, _WaveStrength), (float3x3) gWorldInverseTranspose));
        crest = sampled.w;
    }
    const float2 xz = input.worldPosition.xz;
    const float t = gTime.y;
    normal.xz += 0.05 * float2(sin(dot(xz, float2(3.1, 1.7)) + t * 2.3), sin(dot(xz, float2(-1.3, 2.9)) + t * 1.9));
    normal = normalize(normal);

    const float3 toCamera = normalize(gCameraPosition - input.worldPosition);
    float3 lightDirection, lightColor;
    FindMainLight(lightDirection, lightColor);

    // 水の深さ（水面から、その奥の不透明なものまで。ビューの奥行きで測る）
    const float2 screenUv = GetNormalizedScreenSpaceUV(input.position.xy);
    const float surfaceDepth = LinearEyeDepth(input.position.z);
    float2 refractedUv = screenUv + normal.xz * _RefractionStrength;
    float sceneDepth = LinearEyeDepth(SampleSceneDepth(refractedUv));
    if (sceneDepth < surfaceDepth)
    {
        // ずらした先が水より手前のもの（水から出ている岩など）なら、ずらさない
        refractedUv = screenUv;
        sceneDepth = LinearEyeDepth(SampleSceneDepth(screenUv));
    }
    const float waterDepth = max(sceneDepth - surfaceDepth, 0.0);

    // 屈折: 水の下の色を、深いほど水の色に寄せる（赤から先に吸われる）
    const float3 transmittance = exp(-waterDepth * _Absorption * float3(1.0, 0.45, 0.25));
    const float3 below = SampleSceneColor(refractedUv) * _ShallowColor.rgb;
    float3 color = lerp(_DeepColor.rgb, below, transmittance);

    // 反射: 空の色（Schlick のフレネル。水の F0 は 0.02）
    const float fresnel = 0.02 + 0.98 * pow(1.0 - saturate(dot(normal, toCamera)), 5.0);
    const float3 reflected = reflect(-toCamera, normal);
    const float3 sky = lerp(_SkyColor.rgb * 0.6, _SkyColor.rgb, saturate(reflected.y));
    color = lerp(color, sky, fresnel);

    // 太陽の照り返し
    const float3 halfVector = normalize(-lightDirection + toCamera);
    color += lightColor * pow(saturate(dot(normal, halfVector)), _Shininess) * fresnel * 8.0;

    // 泡: 岸や岩のまわり（浅いところ）と、尖った波の頂
    const float shoreFoam = saturate(1.0 - waterDepth / _ShoreFoamDistance);
    const float crestFoam = saturate((_CrestFoam - crest) * 4.0);
    const float foam = saturate(max(shoreFoam, crestFoam));
    const float foamLighting = saturate(dot(normal, -lightDirection)) * 0.6 + 0.4;
    color = lerp(color, _FoamColor.rgb * lightColor * foamLighting, foam * _FoamColor.a);

    return float4(color, 1.0);
}
