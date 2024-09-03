#define SAMPLE_COUNT 17

cbuffer GbBuffer : register(b0)
{
    float4 sampleOffsets[SAMPLE_COUNT];
    float4 sampleWeights[SAMPLE_COUNT];
}

Texture2D<float4> Texture : register(t0);

sampler TextureSampler : register(s0);

struct InputType
{
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    float4 outputColor = (float4) 0;
    float2 sampleOffset = (float2) 0;
    float sampleWeight = 0;
    
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {        
        sampleOffset = float2(sampleOffsets[i].x, sampleOffsets[i].y);
        sampleWeight = sampleWeights[i].x;
        outputColor += Texture.Sample(TextureSampler, input.tex + sampleOffset) * sampleWeight;
    }
    
    return outputColor;
}