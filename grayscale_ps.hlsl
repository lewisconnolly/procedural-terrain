static const float3 GrayScaleIntensity = { 0.299f, 0.587f, 0.114f };

Texture2D<float4> Texture : register(t0);

sampler TextureSampler : register(s0);

struct InputType
{
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    float4 outputColor = Texture.Sample(TextureSampler, input.tex);
    float intensity = dot(outputColor.rgb, GrayScaleIntensity);

    return float4(intensity.rrr, outputColor.a);
}