
cbuffer BloomBuffer : register (b0)
{
    float bloomThreshold = 0.45f;
    float bloomIntensity = 1.25f;
    float bloomSaturation = 1.0f;
    float sceneIntensity = 1.0f;
    float sceneSaturation = 1.0f;
    float3 padding;
};

Texture2D<float4> Texture : register(t0);

sampler TextureSampler : register(s0);

struct InputType
{
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    float4 color = Texture.Sample(TextureSampler, input.tex);

    return saturate((color - bloomThreshold) / (1 - bloomThreshold));
}

