#include "pch.h"
#include "PostProcessing.h"

PostProcessing::PostProcessing()
{
    m_GbBlurAmount = 2.0;

    m_bloomThreshold = 0.45f;
    m_bloomIntensity = 10.0f;
    m_bloomSaturation = 1.0f;
    m_sceneIntensity = 1.0f;
    m_sceneSaturation = 1.0f;
}


PostProcessing::~PostProcessing()
{
}

bool PostProcessing::SetPixelShader(ID3D11Device* device, WCHAR* psFilename, D3D11_TEXTURE_ADDRESS_MODE textureAddressMode)
{    
    auto pixelShaderBuffer = DX::ReadData(psFilename);
    HRESULT result = device->CreatePixelShader(pixelShaderBuffer.data(), pixelShaderBuffer.size(), NULL, &m_pixelShader);
    if (result != S_OK)
    {
        //if loading failed. 
        return false;
    }

    D3D11_SAMPLER_DESC	samplerDesc;
    // Create a texture sampler state description.
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = textureAddressMode;
    samplerDesc.AddressV = textureAddressMode;
    samplerDesc.AddressW = textureAddressMode;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the texture sampler state.
    device->CreateSamplerState(&samplerDesc, &m_sampleState);

    return true;
}

void PostProcessing::SetGbSampleOffsets(int screenWidth, int screenHeight)
{
    float horizontalPixelSize = 1.0f / screenWidth;
    float verticalPixelSize = 1.0f / screenHeight;

    UINT sampleCount = SAMPLE_COUNT;

    m_GbHorizontalSampleOffsets.resize(SAMPLE_COUNT);
    m_GbVerticalSampleOffsets.resize(SAMPLE_COUNT);
    m_GbHorizontalSampleOffsets[0] = DirectX::SimpleMath::Vector2::Zero;
    m_GbVerticalSampleOffsets[0] = DirectX::SimpleMath::Vector2::Zero;

    for (UINT i = 0; i < SAMPLE_COUNT / 2; i++)
    {
        float sampleOffset = i * 2 + 1.5f;
        float horizontalOffset = horizontalPixelSize * sampleOffset;
        float verticalOffset = verticalPixelSize * sampleOffset;

        m_GbHorizontalSampleOffsets[i * 2 + 1] = DirectX::SimpleMath::Vector2(horizontalOffset, 0.0f);
        m_GbHorizontalSampleOffsets[i * 2 + 2] = DirectX::SimpleMath::Vector2(-horizontalOffset, 0.0f);

        m_GbVerticalSampleOffsets[i * 2 + 1] = DirectX::SimpleMath::Vector2(0.0f, verticalOffset);
        m_GbVerticalSampleOffsets[i * 2 + 2] = DirectX::SimpleMath::Vector2(0.0f, -verticalOffset);
    }
}

void PostProcessing::SetGbSampleWeights()
{
    UINT sampleCount = SAMPLE_COUNT;

    m_GbSampleWeights.resize(SAMPLE_COUNT);
    m_GbSampleWeights[0] = GetGbSampleWeight(0);

    float totalWeight = m_GbSampleWeights[0];
    for (UINT i = 0; i < SAMPLE_COUNT / 2; i++)
    {
        float weight = GetGbSampleWeight((float)i + 1);
        m_GbSampleWeights[i * 2 + 1] = weight;
        m_GbSampleWeights[i * 2 + 2] = weight;
        totalWeight += weight * 2;
    }

    // Normalize the weights so that they sum to one
    for (UINT i = 0; i < m_GbSampleWeights.size(); i++)
    {
        m_GbSampleWeights[i] /= totalWeight;
    }
}

float PostProcessing::GetGbSampleWeight(float x)
{
    return (float)(exp(-(x * x) / (2 * m_GbBlurAmount * m_GbBlurAmount)));
}

bool PostProcessing::CreateGbBuffer(ID3D11Device* device, bool useHorizontalOffsets)
{    
    // Supply the vertex shader constant data.
    GbBufferType GbBufferData;

    if (useHorizontalOffsets)
    {
        for (int i = 0; i < m_GbHorizontalSampleOffsets.size(); i++)
        {
            GbBufferData.sampleOffsets[i].x = m_GbHorizontalSampleOffsets[i].x;
            GbBufferData.sampleOffsets[i].y = m_GbHorizontalSampleOffsets[i].y;
        }
    }
    else
    {
        for (int i = 0; i < m_GbVerticalSampleOffsets.size(); i++)
        {
            GbBufferData.sampleOffsets[i].x = m_GbVerticalSampleOffsets[i].x;
            GbBufferData.sampleOffsets[i].y = m_GbVerticalSampleOffsets[i].y;
        }
    }

    for (int i = 0; i < m_GbSampleWeights.size(); i++)
    {
        GbBufferData.sampleWeights[i].x = m_GbSampleWeights[i];
    }    

    // Fill in a buffer description.
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.ByteWidth = sizeof(GbBufferType);    
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = &GbBufferData;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    // Create the buffer.
    HRESULT result = device->CreateBuffer(&cbDesc, &InitData, &m_GbBuffer);    

    if (result != S_OK)
        return false;

    return true;
}

bool PostProcessing::CreateBloomBuffer(ID3D11Device* device)
{
    // Supply the vertex shader constant data.
    BloomBufferType bloomBufferData;

    bloomBufferData.bloomIntensity = m_bloomIntensity;
    bloomBufferData.bloomSaturation = m_bloomSaturation;
    bloomBufferData.bloomThreshold = m_bloomThreshold;
    bloomBufferData.sceneIntensity = m_sceneIntensity;
    bloomBufferData.sceneSaturation = m_sceneSaturation;    
    
    bloomBufferData.padding = DirectX::XMFLOAT3(1.0, 1.0, 1.0);    

    // Fill in a buffer description.
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.ByteWidth = sizeof(BloomBufferType);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = &bloomBufferData;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    // Create the buffer.
    HRESULT result = device->CreateBuffer(&cbDesc, &InitData, &m_bloomBuffer);

    if (result != S_OK)
        return false;

    return true;
}

ID3D11Buffer* PostProcessing::GetGbBuffer()
{
    return m_GbBuffer;
}

ID3D11Buffer* PostProcessing::GetBloomBuffer()
{
    return m_bloomBuffer;
}

ID3D11SamplerState* PostProcessing::GetSampleState()
{
    return m_sampleState;
}

ID3D11PixelShader* PostProcessing::GetPixelShader()
{
    return m_pixelShader.Get();
}

float* PostProcessing::GetBlurAmount()
{
    return &m_GbBlurAmount;
}

void PostProcessing::SetBlurAmount(float blurAmount)
{
    m_GbBlurAmount = blurAmount;
}

float* PostProcessing::GetBloomIntensity()
{
    return &m_bloomIntensity;
}