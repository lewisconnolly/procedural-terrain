// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D snowTexture : register(t0);
Texture2D rockTexture : register(t1);

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
};

float4 main(InputType input) : SV_TARGET
{
    float4 snowColor;
    float4 rockColor;
    float slope;
    float blendAmount;
        
    float4 textureColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;

    // Sample the grass color from the texture using the sampler at this texture coordinate location.
    snowColor = snowTexture.Sample(SampleType, input.tex);
    
    // Sample the rock color from the texture using the sampler at this texture coordinate location.
    rockColor = rockTexture.Sample(SampleType, input.tex);
    
    // Calculate the slope of this point.
    slope = 1.0f - input.normal.y;
    
    // Determine which texture to use based on height.
    if (slope < 0.2)
    {
        blendAmount = slope / 0.2f;
        textureColor = lerp(snowColor, rockColor, blendAmount);
    }
	
    if (slope >= 0.2f)
    {
        textureColor = rockColor;
    }
    
	// Invert the light direction for calculations.
    lightDir = normalize(input.position3D - lightPosition);

	// Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.normal, -lightDir));

	// Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
    color = ambientColor + (diffuseColor * lightIntensity); //adding ambient
    color = saturate(color);

    // Multiply the texture color and the final light color to get the result
    color = color * textureColor;

    return color;
}

