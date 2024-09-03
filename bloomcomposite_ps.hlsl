static const float3 GrayScaleIntensity = { 0.299f, 0.587f, 0.114f };

cbuffer BloomBuffer : register(b0)
{
    float bloomThreshold = 0.45f;
    float bloomIntensity = 1.25f;
    float bloomSaturation = 1.0f;
    float sceneIntensity = 1.0f;
    float sceneSaturation = 1.0f;
    float3 padding;
};

Texture2D<float4> Texture : register(t0);
Texture2D<float4> BloomTexture : register(t1);

sampler TextureSampler : register(s0);

float4 AdjustSaturation(float4 color, float saturation)
{
    float intensity = dot(color.rgb, GrayScaleIntensity);

    return float4(lerp(intensity.rrr, color.rgb, saturation), color.a);
}

struct InputType
{
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    float4 sceneColor = Texture.Sample(TextureSampler,input.tex);
    float4 bloomColor = BloomTexture.Sample(TextureSampler,input.tex);

    sceneColor = AdjustSaturation(sceneColor, sceneSaturation) * sceneIntensity;
    bloomColor = AdjustSaturation(bloomColor, bloomSaturation) * bloomIntensity;

    sceneColor *= (1 - saturate(bloomColor));

    return sceneColor + bloomColor;
}