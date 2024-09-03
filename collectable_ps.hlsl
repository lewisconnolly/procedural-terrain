// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightPosition;
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 position3D : TEXCOORD2;
    float noise: NOISE;
};

// Function from https://iquilezles.org/articles/palettes/
float3 cosPalette(float t, float3 a, float3 b, float3 c, float3 d)
{
    return a + b * cos(6.28318 * (c * t + d));
}

float4 main(InputType input) : SV_TARGET
{
    float4 textureColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;
    
	// Collectables have uniform brightness (not affected by scene directional light)
    lightIntensity = input.normal;    

	// Init ouput color as ambient
    color = ambientColor; 

    // Color change based on normal length
    float distort = lightIntensity * input.noise * 2.5;
    float3 brightness = float3(0.25, 0.25, 0.25);
    float3 contrast = float3(0.25, 0.25, 0.25);
    float3 oscilation = float3(1.0, 1.0, 1.0);
    float3 phase = float3(0.9, 0.33, 0.67);
    
    color = color + float4(cosPalette(distort, brightness, contrast, oscilation, phase), color.a);    
    
    return color;
}

