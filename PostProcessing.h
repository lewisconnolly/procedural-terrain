#pragma once

constexpr auto SAMPLE_COUNT = 17;

#include "DeviceResources.h"

class PostProcessing
{
public:
	PostProcessing();
	~PostProcessing();

	bool SetPixelShader(ID3D11Device* device, WCHAR* psFilename, D3D11_TEXTURE_ADDRESS_MODE textureAddressMode); //Loads the pixel Shader pair	

	ID3D11SamplerState* GetSampleState();
	ID3D11PixelShader* GetPixelShader();

	// Gaussian blur functions
	void SetGbSampleOffsets(int screenWidth, int screenHeight);
	void SetGbSampleWeights();
	float GetGbSampleWeight(float x);
	bool CreateGbBuffer(ID3D11Device* device, bool useHorizontalOffsets);
	ID3D11Buffer* GetGbBuffer();
	float* GetBlurAmount();
	void SetBlurAmount(float blurAmount);

	// Bloom functions
	bool CreateBloomBuffer(ID3D11Device* device);
	ID3D11Buffer* GetBloomBuffer();
	float* GetBloomIntensity();

private:
	// Type for Gaussian blur pixel shader constant buffer
	// Uses XMFLOAT4 for both properites despite sample offsets being Vector2 and weights being single floats
	// to avoid buffer alignment issue and pixel shader accessing blank areas of memory (each element of an
	// array in a constant buffer uses a 16-byte block regardless of memory requirement)
	struct GbBufferType
	{
		DirectX::XMFLOAT4 sampleOffsets[SAMPLE_COUNT];
		DirectX::XMFLOAT4 sampleWeights[SAMPLE_COUNT];
	};

	// Type for bloom pixel shaders' constant buffer 
	struct BloomBufferType
	{
		float bloomThreshold;
		float bloomIntensity;
		float bloomSaturation;
		float sceneIntensity;
		float sceneSaturation;
		DirectX::XMFLOAT3 padding;
	};

	Microsoft::WRL::ComPtr<ID3D11PixelShader>								m_pixelShader;
	ID3D11SamplerState*														m_sampleState;
		
	// Gaussian blur data variables
	ID3D11Buffer*															m_GbBuffer;
	std::vector<DirectX::XMFLOAT2>											m_GbHorizontalSampleOffsets;
	std::vector<DirectX::XMFLOAT2>											m_GbVerticalSampleOffsets;
	std::vector<float>														m_GbSampleWeights;
	float																	m_GbBlurAmount;

	// Bloom data variables
	ID3D11Buffer*															m_bloomBuffer;
	float																	m_bloomThreshold;
	float																	m_bloomIntensity;
	float 																	m_bloomSaturation;
	float 																	m_sceneIntensity;
	float 																	m_sceneSaturation;
};

