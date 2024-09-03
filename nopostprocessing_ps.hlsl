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
    
    return outputColor;
}