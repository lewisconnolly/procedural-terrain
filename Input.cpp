#include "pch.h"
#include "Input.h"


Input::Input()
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_mouse->SetMode(DirectX::Mouse::MODE_RELATIVE);
	m_quitApp = false;

	m_GameInput.forward				= false;
	m_GameInput.back				= false;
	m_GameInput.right				= false;
	m_GameInput.left				= false;
	m_GameInput.rotRight			= false;
	m_GameInput.rotLeft				= false;
	m_GameInput.rotUp				= false;
	m_GameInput.rotDown				= false;
	m_GameInput.sprint				= false;
	m_GameInput.generate			= false;
	m_GameInput.smooth				= false;
	m_GameInput.showCursor			= false;
	m_GameInput.spawnCollectables	= false;
	m_GameInput.shoot				= false;
	m_GameInput.mouseDelta;
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();	//updates the basic keyboard state
	m_KeyboardTracker.Update(kb);		//updates the more feature filled state. Press / release etc. 
	auto mouse = m_mouse->GetState();   //updates the basic mouse state
	m_MouseTracker.Update(mouse);		//updates the more advanced mouse state. 

	// Get change in mouse movement
	if (!m_GameInput.showCursor)
	{
		m_GameInput.mouseDelta = DirectX::SimpleMath::Vector3(float(mouse.x), float(mouse.y), 0.f);
	}

	// Reset mouse x and y
	m_mouse->GetState();

	if (kb.Escape)// check has escape been pressed.  if so, quit out. 
	{
		m_quitApp = true;
	}

	//A key
	if (kb.A)				m_GameInput.left = true;
	else					m_GameInput.left = false;
	
	//D key
	if (kb.D)				m_GameInput.right = true;
	else					m_GameInput.right = false;

	//W key
	if (kb.W)				m_GameInput.forward	 = true;
	else					m_GameInput.forward = false;

	//S key
	if (kb.S)				m_GameInput.back = true;
	else					m_GameInput.back = false;

	//J key
	if (kb.J)				m_GameInput.rotLeft = true;
	else					m_GameInput.rotLeft = false;

	//L key
	if (kb.L)				m_GameInput.rotRight = true;
	else					m_GameInput.rotRight = false;

	//I key
	if (kb.I)				m_GameInput.rotUp = true;
	else					m_GameInput.rotUp = false;

	//K key
	if (kb.K)				m_GameInput.rotDown = true;
	else					m_GameInput.rotDown = false;

	//space
	if (kb.Space)			m_GameInput.generate = true;
	else					m_GameInput.generate = false;

	//F key
	if (kb.F)				m_GameInput.smooth = true;
	else					m_GameInput.smooth = false;

	//F key
	if (kb.M)				m_GameInput.runMidpointDisplacement = true;
	else					m_GameInput.runMidpointDisplacement = false;	

	//Shift key
	if (kb.LeftShift
		|| kb.RightShift)	m_GameInput.sprint = true;
	else					m_GameInput.sprint = false;

	//Ctrl key
	if (kb.LeftControl)		m_GameInput.showCursor = true;

	//Right click
	if (mouse.rightButton)	m_GameInput.showCursor = false;

	//Left click
	if (mouse.leftButton)	m_GameInput.shoot = true;
	else					m_GameInput.shoot = false;

	//C key
	if (kb.C)				m_GameInput.spawnCollectables = true;
	else					m_GameInput.spawnCollectables = false;
}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}

void Input::setMouseMode(DirectX::Mouse::Mode mode)
{
	// Get change in mouse movement
	m_mouse->SetMode(mode);
	m_mouse->IsVisible()
		? m_mouse->SetVisible(false)
		: m_mouse->SetVisible(true);
}