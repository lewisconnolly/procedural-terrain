#include "ClassicNoise.hlsli"

ClassicNoise classicNoise;

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer TimeBuffer : register(b1)
{
    float time;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 position3D : TEXCOORD2;
    float noise: NOISE;
};

float3x3 rotation3dY(float rad)
{
    float c = cos(rad);
    float s = sin(rad);
    return float3x3(
        c, 0.0, -s,
        0.0, 1.0, 0.0,
        s, 0.0, c
    );
}

float3 rotateY(float3 v, float angle)
{
    return mul(rotation3dY(angle), v);
}

OutputType main(InputType input)
{
    OutputType output;    
    
    float noiseStrength = 0.15;
    float frequency = 1.0;
    float amplitude = 3.0;
    
    // Get noise value from Perlin noise function
    float3 noiseInput = input.normal.xyz + time;
    double noise = classicNoise.noise(noiseInput.x, noiseInput.y, noiseInput.z) * noiseStrength;
    
    float displacement = (float)noise;
    
    // Pass displacement value to pixel shader
    output.noise = displacement;
    
    input.position.x = input.position.x + input.normal.x * displacement;
    input.position.y = input.position.y + input.normal.y * displacement;
    input.position.z = input.position.z + input.normal.z * displacement;
    
    input.position.w = 1.0f;

    // Create a sine wave from top to bottom of the sphere  
    float angle = sin(input.tex.y * frequency + time) * amplitude;
    input.position.xyz = rotateY(input.position.xyz, angle);   
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;

	 // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.normal, (float3x3) worldMatrix);
	
    // Normalize the normal vector.
    output.normal = normalize(output.normal);

	// world position of vertex (for point light)
    output.position3D = (float3) mul(input.position, worldMatrix);
    
    return output;
}