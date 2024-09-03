Texture2D<float4> Texture : register(t0);

sampler TextureSampler : register(s0);

struct InputType
{
    float4 colour : COLOR0;
	float2 tex : TEXCOORD0;
};


float4 main(InputType input) : SV_TARGET
{
    float4 outputColor = Texture.Sample(TextureSampler, input.tex);    
    
    input.tex *= 1.0 - input.tex;
    float vig = input.tex.x * input.tex.y * 5.0;
    vig = pow(vig, 0.75);
    outputColor =  outputColor + vig;           
    outputColor.r *= 1.5;
    
    return outputColor;
    //return float4(vig.rrr, outputColor.a);
}