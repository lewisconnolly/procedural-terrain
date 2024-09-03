//
// Game.h
//
#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shader.h"
#include "modelclass.h"
#include "Light.h"
#include "Input.h"
#include "Camera.h"
#include "RenderTexture.h"
#include "Terrain.h"
#include "PostProcessing.h"
#include "SkyboxEffect.h"
#include "Collectable.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
#ifdef DXTK_AUDIO
    void NewAudioDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
	
private:

	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	}; 

    struct Ray
    {
        DirectX::XMFLOAT3 startPoint;
        DirectX::XMFLOAT3 rayDirection;
        float t;
    };

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
	void SetupGUI();
    void InitialiseTerrain();
    void InitialiseCollectables();
    
    // Collision detection
    bool RaySphereIntersectTest(SimpleMath::Vector3 rayOrigin, SimpleMath::Vector3 rayDir, SimpleMath::Vector3 sphereCenter, float radius);
    void CheckCollectablesCollision(float time);    
    
    // Post-processing
    void RenderWithoutPostProcessing(ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView);
    void ApplyGrayscaleFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView);
    void ApplyVignette(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView);
    void ApplyGaussianBlurFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView);
    void ApplyBloomFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView);

    // Utility functions
    //float lerp(float begin, float end, float t);    

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Window Params
    int                                     m_screenWidth;
    int                                     m_screenHeight;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	//input manager. 
	Input									m_input;
	InputCommands							m_gameInputCommands;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;	
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

	// Scene Objects
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;
	std::unique_ptr<DirectX::GeometricPrimitive>                            m_testmodel;

	//lights
	Light																	m_Light;

	//Cameras
	Camera																	m_Camera01;

	//textures 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_snowDiffuseTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_rockDiffuseTexture;

	//Shaders
	Shader																	m_BasicShaderPair;
	Shader																	m_TerrainShaderPair;
	Shader																	m_CollectableShaderPair;

	//Scene. 
	Terrain																	m_Terrain;
	ModelClass																m_BasicModel;
	ModelClass																m_BasicModel2;
	ModelClass																m_BasicModel3;    

	//RenderTextures
	RenderTexture*															m_FirstRenderPass;
	RenderTexture*															m_SecondRenderPass;
	RECT																	m_fullscreenRect;
	RECT																	m_CameraViewRect;

    //Terrain Params
    float                                                                   m_mpdAmplitude;
    float                                                                   m_mpdAmplitudeLimit;
    float                                                                   m_mpd_dHeight;
    int                                                                     m_mpdTileWidth;
    int                                                                     m_mpdNumRows;
    int                                                                     m_mpdTilesPerRow;
    int                                                                     m_terrainWidth;
    int                                                                     m_terrainHeight;
    //float                                                                 m_terrainScaleFactor;

    //Post-processing
    PostProcessing                                                          m_postProcessing;
    int                                                                     m_postProcessingMode;
    RenderTexture*                                                          m_bloomGlowMap;
    RenderTexture*                                                          m_bloomBlurredGlowMap1;
    RenderTexture*                                                          m_bloomBlurredGlowMap2;

    //Skybox
    std::unique_ptr<DirectX::GeometricPrimitive>                            m_sky;
    std::unique_ptr<SkyboxEffect>                                           m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_skyInputLayout;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_cubemap;
	
    //Collision
    bool                                                                    m_terrainCollisionResult;
    DirectX::SimpleMath::Vector3                                            m_terrainPoint;

    //Collectables
    Collectable                                                             collectable;
    std::vector<Collectable>                                                m_collectables;
    int                                                                     m_numCollectables;
    int                                                                     m_numCollectableCollisions;
    int                                                                     m_numCollectableIntersections;

    //Crosshair    
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_crosshair;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_crosshairNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_crosshairHover;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_crosshairHit;
    bool                                                                    m_justHit;
    float                                                                   m_lastHitTime;
    float                                                                   m_crossHairScale;

    //Gameplay
    int                                                                     m_score;



#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif
    

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
    DirectX::SimpleMath::Matrix                                             m_view;
    DirectX::SimpleMath::Matrix                                             m_projection;
};