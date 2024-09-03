//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <vector>

//toreorganise
#include <fstream>

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    m_screenWidth = 2560;
    m_screenHeight = 1600;
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    // Terrain params
    m_mpdTileWidth = 513;
    m_mpdTilesPerRow = 3;
    m_mpdNumRows = 3;
   
    m_mpdAmplitudeLimit = 10.0;
    m_mpdAmplitude = 9.0;

    // Post-processing
    m_postProcessingMode = 0;
    m_postProcessing.SetGbSampleOffsets(m_screenWidth, m_screenHeight);
    m_postProcessing.SetGbSampleWeights();

    // Collectables
    m_numCollectables = 10;

    // Crosshair
    m_crosshair = m_crosshairNormal;
    m_justHit = false;
    m_lastHitTime = 0;
    m_crossHairScale = 1.0;

    // Gameplay
    m_score = 0;

    /* Initialise rendering resource */
    m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    
    InitialiseTerrain();
    
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = m_screenWidth;
	m_fullscreenRect.bottom = m_screenHeight;

	m_CameraViewRect.left = m_screenWidth - 300;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = m_screenWidth;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.0f, 0.5f, 0.5f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light.setPosition((float)(m_terrainWidth / 2) * m_Terrain.m_scalingFactor, 2000.0f, (float)(m_terrainHeight / 2) * m_Terrain.m_scalingFactor);
	m_Light.setDirection(-90.0f, 0.0f, 0.0f);

	//setup camera
	m_Camera01.setPosition(Vector3((float)(m_terrainWidth / 2) * m_Terrain.m_scalingFactor, 1000.0f, (float)(m_terrainHeight / 2) * m_Terrain.m_scalingFactor));
	m_Camera01.setRotation(Vector3(-50.0f, 50.0f, 0.0f));    

	
#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

	
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{	
	//this is hacky,  i dont like this here.  
	auto device = m_deviceResources->GetD3DDevice();

    // Generate MPD terrain on first frame
    if (timer.GetFrameCount() == 1)
    {
        m_mpd_dHeight = m_mpdTileWidth / (m_mpdAmplitudeLimit - m_mpdAmplitude);
        m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
        m_Terrain.SmoothHeightMap(device, 3);

        InitialiseCollectables();
    }

    // Frame time
    float delta = float(timer.GetElapsedSeconds());    

    // Sprinting
    bool sprinting;
    float sprintModifier;

    if (m_gameInputCommands.sprint)
    {
        sprintModifier = 1000.0f * m_Terrain.m_scalingFactor;
        sprinting = true;
    }
    else
    {
        sprintModifier = 250.0f * m_Terrain.m_scalingFactor;
        sprinting = false;
    }

    // Mouse-based camera control
    Vector3 rotation = m_Camera01.getRotation();
    rotation.x -= m_Camera01.getRotationSpeed() * m_gameInputCommands.mouseDelta.y * delta;
    rotation.y += m_Camera01.getRotationSpeed() * m_gameInputCommands.mouseDelta.x * delta;
    m_Camera01.setRotation(rotation);

    // Show mouse cursor
    if (m_gameInputCommands.showCursor)
    {
        m_input.setMouseMode(DirectX::Mouse::MODE_ABSOLUTE);
    }
    else
    {
        m_input.setMouseMode(DirectX::Mouse::MODE_RELATIVE);
    }
    
    Vector3 position = m_Camera01.getPosition();

    // Movement and camera controls
    if (m_gameInputCommands.left)
    {
        position -= (m_Camera01.getRight() * m_Camera01.getMoveSpeed() * delta * sprintModifier); //substract the right vector        
        m_Camera01.setPosition(position);
    }
    if (m_gameInputCommands.right)
    {
        position += (m_Camera01.getRight() * m_Camera01.getMoveSpeed() * delta * sprintModifier); //add the right vector
        m_Camera01.setPosition(position);
    }
    if (m_gameInputCommands.forward)
    {
        position += (m_Camera01.getForward() * m_Camera01.getMoveSpeed() * delta * sprintModifier); //add the forward vector        
        m_Camera01.setPosition(position);
    }
    if (m_gameInputCommands.back)
    {
        position -= (m_Camera01.getForward() * m_Camera01.getMoveSpeed() * delta * sprintModifier); //substract the forward vector
        m_Camera01.setPosition(position);
    }
    if (m_gameInputCommands.rotLeft)
    {
        rotation.y -= m_Camera01.getRotationSpeed() * delta;
        m_Camera01.setRotation(rotation);
    }
    if (m_gameInputCommands.rotRight)
    {
        rotation.y += m_Camera01.getRotationSpeed() * delta;
        m_Camera01.setRotation(rotation);
    }
    if (m_gameInputCommands.rotUp)
    {
        rotation.x += m_Camera01.getRotationSpeed() * delta;
        m_Camera01.setRotation(rotation);
    }
    if (m_gameInputCommands.rotDown)
    {
        rotation.x -= m_Camera01.getRotationSpeed() * delta;
        m_Camera01.setRotation(rotation);
    }
    

    /* Terrain */

    // Terrain generation
	if (m_gameInputCommands.generate)
	{
		m_Terrain.GenerateHeightMap(device);
	}
    if (m_gameInputCommands.smooth)
    {
        m_Terrain.SmoothHeightMap(device, 1);
    }
    if (m_gameInputCommands.runMidpointDisplacement)
    {        
        //InitialiseTerrain();

        m_mpd_dHeight = m_mpdTileWidth / (m_mpdAmplitudeLimit - m_mpdAmplitude);
        m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
        m_Terrain.SmoothHeightMap(device, 1);

        InitialiseCollectables();
    }    

    // Prevent camera falling through terrain or off edges
    m_Camera01.setPosition(m_Terrain.GetBoundedPosition(position, m_Camera01.getCollisionRadius() * m_Terrain.m_scalingFactor, sprinting, delta));

    /* Collectables */

    // Respawn collectables
    if (m_gameInputCommands.spawnCollectables || m_collectables.size() == 0)
    {
        InitialiseCollectables();
    }    

    CheckCollectablesCollision(timer.GetTotalSeconds());               

    m_Camera01.Update();	//camera update.
    //m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

    m_view = m_Camera01.getCameraMatrix();
    m_world = Matrix::Identity;

	/*create our UI*/
	SetupGUI();

    /*AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::cout << "cam pos(" << m_Camera01.getPosition().x << "," << m_Camera01.getPosition().y << "," << m_Camera01.getPosition().z << ")" <<  "\n";
    std::cout << "cam rot(" << m_Camera01.getRotation().x << "," << m_Camera01.getRotation().y << "," << m_Camera01.getRotation().z << ")" << "\n";*/


#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

void Game::CheckCollectablesCollision(float time)
{
    // Check camera collision with collectables
    m_numCollectableCollisions = 0;
    for (int i = 0; i < m_collectables.size(); i++)
    {
        m_collectables[i].CheckCollision(m_Camera01.getPosition(), m_Camera01.getCollisionRadius(), m_Terrain.m_scalingFactor);

        if (m_collectables[i].m_isHit)
        {
            m_numCollectableCollisions++;
        }
    }

    if (m_score - m_numCollectableCollisions < 0)
    {
        m_score = 0;
    }
    else
    {
        m_score -= m_numCollectableCollisions;
    }

    // Check camera ray collision with collectables    
    m_numCollectableIntersections = 0;
    int idToRemove;
    std::vector<Collectable> m_intersectedCollectables;
    for (int i = 0; i < m_collectables.size(); i++)
    {
        if (RaySphereIntersectTest(m_Camera01.getPosition(), m_Camera01.getForward(), m_collectables[i].m_lastPosition, m_collectables[i].m_radius * m_Terrain.m_scalingFactor))
        {
            m_numCollectableIntersections++;
            //idToRemove = m_collectables[i].m_id;
            m_intersectedCollectables.push_back(m_collectables[i]);
        }
    }

    // If only one collectable hit, remove it, else check for closest to camera
    if (m_intersectedCollectables.size() == 1)
    {
        idToRemove = m_intersectedCollectables[0].m_id;
    }
    else if (m_intersectedCollectables.size() > 1)
    {
        // Sort collectables by nearest to camera    
        for (int i = 0; i < m_intersectedCollectables.size(); i++)
        {
            Vector3 l = m_intersectedCollectables[i].m_lastPosition - m_Camera01.getPosition();
            m_intersectedCollectables[i].m_distanceFromCamera = l.LengthSquared();
        }

        std::vector<Collectable> m_sortedCollectables = m_intersectedCollectables;
        std::sort(m_sortedCollectables.begin(), m_sortedCollectables.end(), [](const Collectable& lhs, const Collectable& rhs)
            {
                return lhs.m_distanceFromCamera < rhs.m_distanceFromCamera;
            });
        
        // remove closest
        idToRemove = m_sortedCollectables[0].m_id;
    }

    // Change cursor based on hovering over collectable or hitting with shot
    // If hit registered, keep hit crosshair visible for half a second
    if (!m_justHit || time - m_lastHitTime >= 0.33)
    {
        if (m_numCollectableIntersections > 0)
        {
            if (m_gameInputCommands.shoot)
            {
                m_crosshair = m_crosshairHit;
                m_crossHairScale = 0.5;
                m_lastHitTime = time;
                m_justHit = true;

                m_collectables.erase(
                    std::remove_if(
                        m_collectables.begin(),
                        m_collectables.end(),
                        [idToRemove](const Collectable& c)
                        {
                            return c.m_id == idToRemove;
                        }
                    ),
                    m_collectables.end());

                m_score++;
            }
            else
            {
                m_crosshair = m_crosshairHover;
                m_crossHairScale = 1.0;
                m_justHit = false;
            }
        }
        else
        {
            m_crosshair = m_crosshairNormal;
            m_crossHairScale = 1.0;
            m_justHit = false;
        }
    }
}

// Algorithm from Real-Time Rendering Fourth Edition - Tomas Akenine-Moller, pg. 958
bool Game::RaySphereIntersectTest(SimpleMath::Vector3 rayOrigin, SimpleMath::Vector3 rayDir, SimpleMath::Vector3 sphereCenter, float radius)
{
    // Vector from ray origin to sphere's center
    Vector3 l = sphereCenter - rayOrigin;
    // Project l along ray direction
    float s = l.Dot(rayDir);
    
    // If the sphere is behind the direction from ray origin and ray origin is outside the sphere, no intersection
    if (s < 0 && l.LengthSquared() > radius * radius)
    {
        return false;
    }

    // Get squared distance from projected ray s to sphere center using the Pythagorean theorem
    float m_sqd = l.LengthSquared() - s * s;

    // If ray center to ray endpoint is greater than radius, then ray outside sphere (no intersection)
    if (m_sqd > radius * radius)
    {
        return false;
    }
    else
    {
        return true;
    }
}

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{	
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
    //auto viewport = m_deviceResources->GetScreenViewport();
    
    SimpleMath::Matrix newPosition = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix newRotation = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix newScale = SimpleMath::Matrix::Identity;

    float totalSecs = float(m_timer.GetTotalSeconds());

    std::wstringstream wss;
    /*  wss << "Camera X: " << m_Camera01.getPosition().x << "\n";
    wss << "Camera Y: " << m_Camera01.getPosition().y << "\n";
    wss << "Camera Z: " << m_Camera01.getPosition().z << "\n";
    wss << "\n";
    wss << "Num intersects: " << m_numCollectableIntersections << "\n";*/
    wss << m_score;

    /* First render pass */

    // Set the render target to be the render to texture.
    m_FirstRenderPass->setRenderTarget(context);
    // Clear the render to texture.
    m_FirstRenderPass->clearRenderTarget(context, 0.0f, 0.0f, 0.05f, 1.0f);

    // Render sky box
    m_effect->SetView(m_view);
    m_sky->Draw(m_effect.get(), m_skyInputLayout.Get());    

	// Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
    //context->RSSetState(m_states->Wireframe());            
	
    // Set up and draw collectables
    //m_BasicShaderPair.EnableShader(context);
    m_CollectableShaderPair.EnableShader(context);

    float shiftY = sin(totalSecs * 100.0 / 20.0) * 20.0; // bob collectables up and down
    for (int i = 0; i < m_collectables.size(); i++)
    {
        SimpleMath::Vector3 centerTranslation = (m_collectables[i].m_spawnPosition + SimpleMath::Vector3(0, shiftY, 0)) * m_Terrain.m_scalingFactor;

        m_world = SimpleMath::Matrix::Identity; //set world back to identity	
        //newPosition = SimpleMath::Matrix::CreateTranslation((m_collectables[i].m_spawnPosition + SimpleMath::Vector3(0, shiftY, 0)) * m_Terrain.m_scalingFactor);
        newPosition = SimpleMath::Matrix::CreateTranslation(centerTranslation);
        newRotation = SimpleMath::Matrix::CreateRotationY(totalSecs); // rotate around own y-axis
        newScale = SimpleMath::Matrix::CreateScale(m_collectables[i].m_radius * 2.0 * m_Terrain.m_scalingFactor);
        m_world = m_world * newScale * newRotation * newPosition;
        
        //m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
        m_CollectableShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
        m_CollectableShaderPair.AddTimeCBuffer(device, context, totalSecs);
        m_collectables[i].m_collectableSphere.Render(context);

        m_collectables[i].m_lastPosition = centerTranslation;
    }    

	// setup and draw terrain	
    m_world = SimpleMath::Matrix::Identity; //set world back to identity	
    newPosition = SimpleMath::Matrix::CreateTranslation(0, 0, 0);
    newScale = SimpleMath::Matrix::CreateScale(m_Terrain.m_scalingFactor);
    m_world = m_world * newScale * newPosition;

    m_TerrainShaderPair.EnableShader(context);
    m_TerrainShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_snowDiffuseTexture.Get(), m_rockDiffuseTexture.Get());
	m_Terrain.Render(context);

    /* End of first render pass */

    if (m_numCollectableCollisions > 0)
    {
        ApplyVignette(device, context, renderTargetView, depthTargetView);
    }

    // Post-processing
    switch (m_postProcessingMode)
    {
    case 0:
        RenderWithoutPostProcessing(context, renderTargetView, depthTargetView);
        break;
    case 1:
        ApplyGrayscaleFilter(device, context, renderTargetView, depthTargetView);
        break;
    case 2:
        ApplyGaussianBlurFilter(device, context, renderTargetView, depthTargetView);
        break;
    case 3:
        ApplyBloomFilter(device, context, renderTargetView, depthTargetView);
        break;
    default:
        RenderWithoutPostProcessing(context, renderTargetView, depthTargetView);
        break;
    }          

    // Draw Text to the screen
    m_sprites->Begin(SpriteSortMode_Deferred);
    m_font->DrawString(
        m_sprites.get(),
        wss.str().c_str(),
        XMFLOAT2(m_screenWidth - 300, 100),
        Colors::MediumVioletRed,
        0,
        XMFLOAT2(0,0),
        3.0f,
        SpriteEffects_None,
        1.0f);
    m_sprites->End();

    // Draw crosshair on top of everything
    m_sprites->Begin(SpriteSortMode_Deferred, m_states->AlphaBlend());
    m_sprites->Draw(
        m_crosshair.Get(), // sprite texture
        XMFLOAT2(m_screenWidth / 2.0, m_screenHeight / 2.0), // screen position
        nullptr, // default (use whole texture)
        Colors::White, // no tint
        0, // no rotate
        XMFLOAT2(64.0, 64.0), // position based on center pixel of texture (crosshair tex is 128x128)
        XMFLOAT2(m_crossHairScale, m_crossHairScale), // hit crosshair smaller
        SpriteEffects_None, // no effect
        1.0 // layer depth 1
    ); 
    m_sprites->End();

	// Render GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	    

    // Show the new frame.
    m_deviceResources->Present();
}

void Game::RenderWithoutPostProcessing(ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView)
{
    // Reset the render target back to the original back buffer and not the render to texture anymore.	
    context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);

    // Draw render to texture target on full screen sprite using post-processing pixel shader and sample state
    m_sprites->Begin();
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();
}

void Game::ApplyGrayscaleFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView)
{           
    // Reset the render target back to the original back buffer and not the render to texture anymore.	
    context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
    
    // Set post-processing pixel shader
    m_postProcessing.SetPixelShader(device, L"grayscale_ps.cso", D3D11_TEXTURE_ADDRESS_WRAP);

    // Draw render to texture target on full screen sprite using post-processing pixel shader and sample state
    m_sprites->Begin(SpriteSortMode_Immediate,
    nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
    {
        context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
    });
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();
}

void Game::ApplyVignette(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView)
{
    // Set post-processing pixel shader
    m_postProcessing.SetPixelShader(device, L"vignette_ps.cso", D3D11_TEXTURE_ADDRESS_WRAP);

    // Draw render to texture target on full screen sprite using post-processing pixel shader and sample state
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
        });
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();
}


void Game::ApplyGaussianBlurFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView)
{    
    // Set render target to different texture for second render pass (required for gaussian blur effect)
    m_SecondRenderPass->setRenderTarget(context);
    // Clear the render to texture.
    m_SecondRenderPass->clearRenderTarget(context, 0.0f, 0.0f, 0.05f, 1.0f);
    
    // Set post-processing pixel shader
    m_postProcessing.SetPixelShader(device, L"gaussianblur_ps.cso", D3D11_TEXTURE_ADDRESS_CLAMP);

    // Create gaussian blur pixel shader constant buffer using horizontal offsets
    m_postProcessing.CreateGbBuffer(device, true);
    ID3D11Buffer* buffer = m_postProcessing.GetGbBuffer();

    // Draw render to texture target on full screen sprite using post-processing pixel shader, sample state and constant buffer
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);                        
            context->PSSetConstantBuffers(0, 1, &buffer);            
        });
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();

    // Reset the render target back to the original back buffer and not the render to texture anymore.	
    context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);

    // Create gaussian blur pixel shader constant buffer using vertical offsets
    m_postProcessing.CreateGbBuffer(device, false);
    buffer = m_postProcessing.GetGbBuffer();

    // Draw second render pass texture on full screen sprite with post-processing pixel shader, sample state and constant buffer
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
            context->PSSetConstantBuffers(0, 1, &buffer);            
        });
    m_sprites->Draw(m_SecondRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();
}

void Game::ApplyBloomFilter(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthTargetView)
{
    /* 1) Create bloom map */

    // Set render target to bloom glow map texture
    m_bloomGlowMap->setRenderTarget(context);
    // Clear the render to texture.
    m_bloomGlowMap->clearRenderTarget(context, 0.0f, 0.0f, 0.05f, 1.0f);

    // Set post-processing pixel shader to extract bloom map
    m_postProcessing.SetPixelShader(device, L"bloomextract_ps.cso", D3D11_TEXTURE_ADDRESS_WRAP);

    // Create bloom pixel shader constant buffer
    m_postProcessing.CreateBloomBuffer(device);
    ID3D11Buffer* bloomBuffer = m_postProcessing.GetBloomBuffer();

    // Process first render pass with bloomextract PS to create bloom glow map in m_bloomGlowMap render to texture
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
            context->PSSetConstantBuffers(0, 1, &bloomBuffer);
        });
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();


    /* 2) Blur the bloom map */

    // Set render target to bloom blurred glow map texture 1 (two passes required for GB)
    m_bloomBlurredGlowMap1->setRenderTarget(context);
    // Clear the render to texture.
    m_bloomBlurredGlowMap1->clearRenderTarget(context, 0.0f, 0.0f, 0.05f, 1.0f);

    // Create gaussian blur pixel shader constant buffer using horizontal offsets
    m_postProcessing.CreateGbBuffer(device, true);
    ID3D11Buffer* gbBuffer = m_postProcessing.GetGbBuffer();

    // Set post-processing pixel shader for gaussian blur
    m_postProcessing.SetPixelShader(device, L"gaussianblur_ps.cso", D3D11_TEXTURE_ADDRESS_CLAMP);

    // Blur the bloom glow map texture using gaussian blur horizontal
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
            context->PSSetConstantBuffers(0, 1, &gbBuffer);
        });
    m_sprites->Draw(m_bloomGlowMap->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();

    // Set render target to bloom blurred glow map texture 2
    m_bloomBlurredGlowMap2->setRenderTarget(context);
    // Clear the render to texture.
    m_bloomBlurredGlowMap2->clearRenderTarget(context, 0.0f, 0.0f, 0.05f, 1.0f);

    // Create gaussian blur pixel shader constant buffer using vertical offsets
    m_postProcessing.CreateGbBuffer(device, false);
    gbBuffer = m_postProcessing.GetGbBuffer();

    // Blur the bloom glow map texture 1 using gaussian blur vertical
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
            context->PSSetConstantBuffers(0, 1, &gbBuffer);
        });
    m_sprites->Draw(m_bloomBlurredGlowMap1->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();


    /* 3) Process first render pass using bloom effect */

    // Reset the render target back to the original back buffer and not the render to texture anymore.	
    context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);

    // Set post-processing pixel shader to bloom effect
    m_postProcessing.SetPixelShader(device, L"bloomcomposite_ps.cso", D3D11_TEXTURE_ADDRESS_WRAP);

    // Get shader resource view of fully blurred glow map texture
    ID3D11ShaderResourceView* bloomSRV = m_bloomBlurredGlowMap2->getShaderResourceView();

    // Create bloom effect on first render pass using bloom composite PS and blurred glow map texture
    m_sprites->Begin(SpriteSortMode_Immediate,
        nullptr, m_postProcessing.GetSampleState(), nullptr, nullptr, [=]
        {
            context->PSSetShader(m_postProcessing.GetPixelShader(), 0, 0);
            context->PSSetConstantBuffers(0, 1, &bloomBuffer);
            context->PSSetShaderResources(1, 1, &bloomSRV);
        });
    m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
    m_sprites->End();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = m_screenWidth;
    height = m_screenHeight;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);	        

    //Set up sky box
    m_sky = GeometricPrimitive::CreateGeoSphere(context, 2.f, 3, false /*invert for being inside the shape*/);
    m_effect = std::make_unique<SkyboxEffect>(device);
    m_sky->CreateInputLayout(m_effect.get(), m_skyInputLayout.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"skybox.dds", nullptr, m_cubemap.ReleaseAndGetAddressOf());
    m_effect->SetTexture(m_cubemap.Get());

	//Set up models   
    //m_BasicModel.InitializeSphere(device);
	//m_BasicModel2.InitializeModel(device,"drone.obj");
	//m_BasicModel3.InitializeBox(device, 10.0f, 0.1f, 10.0f);	//box includes dimensions

	//load and set up our Vertex and Pixel Shaders
	m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
    m_TerrainShaderPair.InitStandard(device, L"terrain_vs.cso", L"terrain_ps.cso");
    m_CollectableShaderPair.InitStandard(device, L"collectable_vs.cso", L"collectable_ps.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"seafloor.dds",		nullptr,	m_texture1.ReleaseAndGetAddressOf());
	//CreateDDSTextureFromFile(device, L"EvilDrone_Diff.dds", nullptr,	m_texture2.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"snow_diffuse.dds", nullptr, m_snowDiffuseTexture.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"rock_diffuse.dds", nullptr, m_rockDiffuseTexture.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"crosshair_normal.dds", nullptr, m_crosshairNormal.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"crosshair_hover.dds", nullptr, m_crosshairHover.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"crosshair_hit.dds", nullptr, m_crosshairHit.ReleaseAndGetAddressOf());

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, m_screenWidth, m_screenHeight, 1, 2);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 
	m_SecondRenderPass = new RenderTexture(device, m_screenWidth, m_screenHeight, 1, 2);

    //Init bloom render to textures
    m_bloomGlowMap = new RenderTexture(device, m_screenWidth, m_screenHeight, 1, 2);
    m_bloomBlurredGlowMap1 = new RenderTexture(device, m_screenWidth, m_screenHeight, 1, 2);
    m_bloomBlurredGlowMap2 = new RenderTexture(device, m_screenWidth, m_screenHeight, 1, 2);
}

void Game::InitialiseTerrain()
{
    auto device = m_deviceResources->GetD3DDevice();
    
    //set up our terrain
    m_terrainWidth = m_mpdTileWidth * m_mpdTilesPerRow;
    m_terrainHeight = m_mpdTileWidth * m_mpdNumRows;
    m_Terrain.Initialize(device, m_terrainWidth, m_terrainHeight, 10.0);
}

void Game::InitialiseCollectables()
{
    auto device = m_deviceResources->GetD3DDevice();
    
    m_collectables.clear();
    
    Collectable collectable;
    for (int i = 0; i < m_numCollectables; i++)
    {        
        collectable.Initialise(device, &m_Terrain, i, m_numCollectables);
        m_collectables.push_back(collectable);
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        1000000.0f
    );

    m_effect->SetProjection(m_projection);
}

void Game::SetupGUI()
{
    auto device = m_deviceResources->GetD3DDevice();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Sin/Cos Wave Params");
		ImGui::SliderFloat("Wave Amplitude",	m_Terrain.GetAmplitude(), 0.0f, 10.0f);
		ImGui::SliderFloat("Wavelength",		m_Terrain.GetWavelength(), 0.0f, 1.0f);
    ImGui::End();
        
    ImGui::Begin("Random Height Params");
        ImGui::Checkbox("Enable Random Height", m_Terrain.GetRandomHeightEnabledState());
        ImGui::SliderFloat("Maximum Height",    m_Terrain.GetRandomHeightLimit(), 1.0f, 500.0f);
	ImGui::End();

    ImGui::Begin("Midpoint Displacement (Diamond-square) Params");
        if (ImGui::SliderFloat("Amplitude", &m_mpdAmplitude, 0.0f, m_mpdAmplitudeLimit - 1.0))
        {
            /*m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
            m_Terrain.SmoothHeightMap(device, 1);*/
        }
        if (ImGui::SliderFloat("Roughness", m_Terrain.GetRoughness(), 0.5f, 1.5f))
        {
            /*m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
            m_Terrain.SmoothHeightMap(device, 1);*/
        }
        if (ImGui::SliderInt("Tiles per Row", &m_mpdTilesPerRow, 1, 5))
        {
            InitialiseTerrain();
            m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
            m_Terrain.SmoothHeightMap(device, 1);

        }
        if (ImGui::SliderInt("Number of Rows", &m_mpdNumRows, 1, 5))
        {
            InitialiseTerrain();
            m_Terrain.MidpointDisplacement(device, m_mpd_dHeight, m_mpdTileWidth, m_mpdTilesPerRow, m_mpdNumRows);
            m_Terrain.SmoothHeightMap(device, 1);
        }
    ImGui::End();

    enum PostProcessingMode
    {
        None,
        Grayscale,
        GaussianBlur,
        Bloom
    };

    ImGui::Begin("Post-processing");
        if (ImGui::RadioButton("None", m_postProcessingMode == None)) { m_postProcessingMode = None; }
        if (ImGui::RadioButton("Grayscale", m_postProcessingMode == Grayscale)) { m_postProcessingMode = Grayscale; }
        if (ImGui::RadioButton("Gaussian Blur", m_postProcessingMode == GaussianBlur))
        {
            m_postProcessing.SetBlurAmount(5.0f);
            m_postProcessingMode = GaussianBlur;
        }
        if (ImGui::SliderFloat("Blur Amount", m_postProcessing.GetBlurAmount(), 1.0f, 20.0f)) { m_postProcessing.SetGbSampleWeights(); };
        if (ImGui::RadioButton("Bloom", m_postProcessingMode == Bloom))
        {
            m_postProcessing.SetBlurAmount(20.0f);
            m_postProcessing.SetGbSampleWeights();
            m_postProcessingMode = Bloom;
        }
        ImGui::SliderFloat("Bloom Intensity", m_postProcessing.GetBloomIntensity(), 1.0f, 20.0f);
    ImGui::End();
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
