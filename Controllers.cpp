//
//  Controllers.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 18/12/2016.
//  Copyright © 2016 Stephen Harrison-Daly. All rights reserved.
//

#include "Controllers.h"
#include "Application.h"
#include <string>
#include <windows.h>
#include <XInput.h>

using namespace shd;

Controllers::State::State() :	leftStick({ 0, 0 }),
								rightStick({ 0, 0 }),
								leftTrigger(0),
								rightTrigger(0),
								leftStickXInt(0),
								leftStickYInt(0),
								rightStickXInt(0),
								rightStickYInt(0),
								rumbleStrengthBig(0),
								rumbleStrengthSmall(0),
								isConnected(false)
{
	memset(buttons, 0, sizeof(bool) * BUTTON_MAX);
}

bool Controllers::State::operator==(const State& rhs)
{
	for (int i = 0; i < BUTTON_MAX; i++)
	{
		if (buttons[i] != rhs.buttons[i])
		{
			return false;
		}
	}

	if (leftTrigger - rhs.leftTrigger > 0.05f)
	{
		return false;
	}

	if (rightTrigger - rhs.rightTrigger > 0.05f)
	{
		return false;
	}

	return true;
}

bool Controllers::State::operator!=(const State& rhs)
{
	return !(*this == rhs);
}

bool Controllers::init()
{
	// Get initial state of the controlers
	refreshControllerList();

	return true;
}

void Controllers::update()
{
	// If the application doesn't have window focus, then we ignore inputs
	if (Application::getInstance().hasWindowFocus == false)
	{
		return;
	}

	DWORD dwResult;
	XINPUT_STATE state;

	memcpy(m_controllersPrevious, m_controllers, sizeof(State) * SHD_MAX_CONTROLLERS);
	memcpy(&m_keyboardPrevious, &m_keyboard, sizeof(State));
	memcpy(&m_networkControllerPrevious, &m_networkController, sizeof(State));
	
	// If we're in a network game, then get the next input that was received, from the jitter buffer
	if (Application::getInstance().networkThread.getNetworkState() == NetworkThread::NET_STATE_IN_GAME)
	{
		m_networkController = Application::getInstance().networkThread.getNetJitterBuffer()->getNextState();
	}

	///////////////////////////////////////////////
	// Update controllers states first
	///////////////////////////////////////////////

	for (DWORD i = 0; i< XUSER_MAX_COUNT; i++)
	{
		if (m_controllers[i].isConnected == false)
		{
			continue;
		}

		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// Get the state of the controller from XInput
		dwResult = XInputGetState(i, &state);

		if (dwResult == ERROR_SUCCESS)
		{
			// Trigger value between 0.0f and 1.0f
			m_controllers[i].leftTrigger = (float)state.Gamepad.bLeftTrigger / 255.0f;
			m_controllers[i].rightTrigger = (float)state.Gamepad.bRightTrigger / 255.0f;

			// Get button values
			m_controllers[i].buttons[BUTTON_UP] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? true : false;
			m_controllers[i].buttons[BUTTON_DOWN] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? true : false;
			m_controllers[i].buttons[BUTTON_LEFT] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? true : false;
			m_controllers[i].buttons[BUTTON_RIGHT] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? true : false;
			m_controllers[i].buttons[BUTTON_START] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) ? true : false;
			m_controllers[i].buttons[BUTTON_SELECT] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) ? true : false;
			m_controllers[i].buttons[BUTTON_X] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? true : false;
			m_controllers[i].buttons[BUTTON_CIRCLE] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? true : false;
			m_controllers[i].buttons[BUTTON_SQUARE] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) ? true : false;
			m_controllers[i].buttons[BUTTON_TRIANGLE] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ? true : false;
			m_controllers[i].buttons[BUTTON_L1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? true : false;
			m_controllers[i].buttons[BUTTON_R1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? true : false;
			m_controllers[i].buttons[BUTTON_L3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? true : false;
			m_controllers[i].buttons[BUTTON_R3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? true : false;

			m_controllers[i].buttons[BUTTON_L2] = (m_controllers[i].leftTrigger > 0.5f) ? true : false;
			m_controllers[i].buttons[BUTTON_R2] = (m_controllers[i].rightTrigger > 0.5f) ? true : false;

			// Left stick
			m_controllers[i].leftStickXInt = state.Gamepad.sThumbLX;
			m_controllers[i].leftStickYInt = state.Gamepad.sThumbLY;
			m_controllers[i].leftStick = sticksToVec2f(m_controllers[i].leftStickXInt, m_controllers[i].leftStickYInt);

			// Repeat for right thumb stick
			m_controllers[i].rightStickXInt = state.Gamepad.sThumbRX;
			m_controllers[i].rightStickYInt = state.Gamepad.sThumbRY;
			m_controllers[i].rightStick = sticksToVec2f(m_controllers[i].rightStickXInt, m_controllers[i].rightStickYInt);
		}
	}

	///////////////////////////////////////////////
	// Then udpate keyboard state
	///////////////////////////////////////////////

	m_keyboard.buttons[BUTTON_UP] = (GetAsyncKeyState('W')) ? true : false;
	m_keyboard.buttons[BUTTON_DOWN] = (GetAsyncKeyState('S')) ? true : false;
	m_keyboard.buttons[BUTTON_LEFT] = (GetAsyncKeyState('A')) ? true : false;
	m_keyboard.buttons[BUTTON_RIGHT] = (GetAsyncKeyState('D')) ? true : false;

	if(m_keyboard.buttons[BUTTON_UP] == false)
		m_keyboard.buttons[BUTTON_UP] = (GetAsyncKeyState(VK_UP)) ? true : false;
	if (m_keyboard.buttons[BUTTON_DOWN] == false)
		m_keyboard.buttons[BUTTON_DOWN] = (GetAsyncKeyState(VK_DOWN)) ? true : false;
	if (m_keyboard.buttons[BUTTON_LEFT] == false)
		m_keyboard.buttons[BUTTON_LEFT] = (GetAsyncKeyState(VK_LEFT)) ? true : false;
	if (m_keyboard.buttons[BUTTON_RIGHT] == false)
		m_keyboard.buttons[BUTTON_RIGHT] = (GetAsyncKeyState(VK_RIGHT)) ? true : false;

	m_keyboard.buttons[BUTTON_CIRCLE] = (GetAsyncKeyState(VK_BACK)) ? true : false;
	m_keyboard.buttons[BUTTON_START] = (GetAsyncKeyState(VK_ESCAPE)) ? true : false;
	m_keyboard.buttons[BUTTON_R1] = (GetAsyncKeyState('E')) ? true : false;
	m_keyboard.buttons[BUTTON_X] = (GetAsyncKeyState(VK_RETURN)) ? true : false;
	m_keyboard.buttons[BUTTON_L2] = (GetAsyncKeyState(VK_SPACE)) ? true : false;

	// GetAsyncKeyState checks for the physical button, not the mapped one. So if the user mapped the
	// mouse click, we need to check for the right physical button instead
	if (GetSystemMetrics(SM_SWAPBUTTON) == false)
	{
		m_keyboard.buttons[BUTTON_R2] = (GetAsyncKeyState(VK_LBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_MOUSE_LEFT] = (GetAsyncKeyState(VK_LBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_MOUSE_RIGHT] = (GetAsyncKeyState(VK_RBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_L1] = (GetAsyncKeyState(VK_RBUTTON)) ? true : false;
	}
	else
	{
		m_keyboard.buttons[BUTTON_R2] = (GetAsyncKeyState(VK_RBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_MOUSE_LEFT] = (GetAsyncKeyState(VK_RBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_MOUSE_RIGHT] = (GetAsyncKeyState(VK_LBUTTON)) ? true : false;
		m_keyboard.buttons[BUTTON_L1] = (GetAsyncKeyState(VK_LBUTTON)) ? true : false;
	}

#ifdef _DEBUG
#if 0
	// TODO - remove test
	m_controllers[0].isConnected = true;

	if (Application::getInstance().appState == Application::APP_STATE_IN_MAIN_MENU && shd::getRandomNumber(100) == 0)
	{
		m_controllers[0].buttons[BUTTON_RIGHT] = true;
	}
	else if (Application::getInstance().appState == Application::APP_STATE_IN_MAIN_MENU && shd::getRandomNumber(100) == 1)
	{
		m_controllers[0].buttons[BUTTON_X] = true;
	}
	else if ( Application::getInstance().appState == Application::APP_STATE_IN_GAME_LOCAL && shd::getRandomNumber(100) == 2)
	{
		m_controllers[0].buttons[BUTTON_R2] = true;
	}
	else
	{
		m_controllers[0].buttons[BUTTON_RIGHT] = false;
		m_controllers[0].buttons[BUTTON_R2] = false;
		m_controllers[0].buttons[BUTTON_X] = false;
	}
	// TODO - remove above!
#endif
#endif
}

void Controllers::refreshControllerList()
{
	DWORD dwResult;
	XINPUT_STATE state;

	for (DWORD i = 0; i< XUSER_MAX_COUNT; i++)
	{
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// Get the state of the controller from XInput
		dwResult = XInputGetState(i, &state);

		if (dwResult == ERROR_SUCCESS)
		{
			// Controller is connected 
			m_controllers[i].isConnected = true;
		}
		else
		{
			// Controller is not connected 
			m_controllers[i].isConnected = false;
		}
	}
}

Controllers::State & Controllers::getControllerState(int controllerIndex)
{
	if (controllerIndex == SHD_CONTROLLERS_KEYBOARD_INDEX)
	{
		return m_keyboard;
	}
	else if (controllerIndex == SHD_CONTROLLERS_NETWORK_INDEX)
	{
		return m_networkController;
	}
	else if (controllerIndex >= 0 && controllerIndex < SHD_MAX_CONTROLLERS)
	{
		return m_controllers[controllerIndex];
	}

	SHD_ASSERT(false);
	State tempState;
	memset(&tempState, 0, sizeof(State));
	return tempState;
}

Controllers::State & Controllers::getControllerStatePrevious(int controllerIndex)
{
	if (controllerIndex == SHD_CONTROLLERS_KEYBOARD_INDEX)
	{
		return m_keyboardPrevious;
	}
	else if (controllerIndex == SHD_CONTROLLERS_NETWORK_INDEX)
	{
		return m_networkControllerPrevious;
	}
	else if (controllerIndex >= 0 && controllerIndex < SHD_MAX_CONTROLLERS)
	{
		return m_controllersPrevious[controllerIndex];
	}

	SHD_ASSERT(false);
	State tempState;
	memset(&tempState, 0, sizeof(State));
	return tempState;
}

bool Controllers::isButtonPressed(int controllerIndex, ButtonType button)
{
	if (button < 0 || button > BUTTON_MAX)
	{
		return false;
	}

	if (controllerIndex == SHD_CONTROLLERS_KEYBOARD_INDEX)
	{
		return (m_keyboard.buttons[button] == true && m_keyboardPrevious.buttons[button] == false);
	}

	if (controllerIndex == SHD_CONTROLLERS_NETWORK_INDEX)
	{
		return (m_networkController.buttons[button] == true && m_networkControllerPrevious.buttons[button] == false);
	}

	if (controllerIndex < 0 || controllerIndex > SHD_MAX_CONTROLLERS)
	{
		return false;
	}

	return (m_controllers[controllerIndex].buttons[button] == true && m_controllersPrevious[controllerIndex].buttons[button] == false);
}

bool Controllers::isButtonHeld(int controllerIndex, ButtonType button)
{
	if (button < 0 || button > BUTTON_MAX)
	{
		return false;
	}

	if (controllerIndex == SHD_CONTROLLERS_KEYBOARD_INDEX)
	{
		return m_keyboard.buttons[button];
	}

	if (controllerIndex == SHD_CONTROLLERS_NETWORK_INDEX)
	{
		return m_networkController.buttons[button];
	}

	if (controllerIndex < 0 || controllerIndex > SHD_MAX_CONTROLLERS)
	{
		return false;
	}

	return m_controllers[controllerIndex].buttons[button];
}

bool Controllers::isButtonReleased(int controllerIndex, ButtonType button)
{
	if (button < 0 || button > BUTTON_MAX)
	{
		return false;
	}

	if (controllerIndex == SHD_CONTROLLERS_KEYBOARD_INDEX)
	{
		return (m_keyboard.buttons[button] == false && m_keyboardPrevious.buttons[button] == true);
	}

	if (controllerIndex == SHD_CONTROLLERS_NETWORK_INDEX)
	{
		return (m_networkController.buttons[button] == false && m_networkControllerPrevious.buttons[button] == true);
	}

	if (controllerIndex < 0 || controllerIndex > SHD_MAX_CONTROLLERS)
	{
		return false;
	}

	return (m_controllers[controllerIndex].buttons[button] == false && m_controllersPrevious[controllerIndex].buttons[button] == true);
}

void Controllers::setMousePos(int x, int y)
{
	float width = Application::getInstance().renderer.getScreenWidthFromOS();
	float height = Application::getInstance().renderer.getScreenHeightFromOS();
	float aspectRatio = Application::getInstance().renderer.getScreenAspectRatio();

	m_mousePosPrevious = m_mousePos;

	// Transform mouse position from screen space to world space
	m_mousePos.x = ((kWorldHeight * aspectRatio * x)/ width) - kHalfWorldHeight * aspectRatio;
	m_mousePos.y = -((kWorldHeight * y) / height) + kHalfWorldHeight;

	if (m_mousePosPrevSet == false)
	{
		m_mousePosPrevSet = true;
		m_mousePosPrevious = m_mousePos;
	}
}

void Controllers::setMouseAsRightStick(Vec2f & stickPos)
{
	m_keyboard.rightStickXInt = stickPos.x * INT16_MAX;
	m_keyboard.rightStickYInt = stickPos.y * INT16_MAX;
}

Vec2f Controllers::getMousePos()
{
	return (m_mousePos * Application::getInstance().camera.getZoomScaler()) + Application::getInstance().camera.getPosition();
}

Vec2f Controllers::getMousePosUI()
{
	return m_mousePos + Application::getInstance().camera.getPosition();
}

bool Controllers::hasMousePosChanged()
{
	return (m_mousePos.distanceTo(m_mousePosPrevious) > 0.25f);
}

bool Controllers::isConnected(int controllerIndex)
{
	if (controllerIndex < 0 || controllerIndex >= SHD_MAX_CONTROLLERS)
	{
		return false;
	}

	return m_controllers[controllerIndex].isConnected;
}

Vec2f Controllers::sticksToVec2f(int16_t stickX, int16_t stickY)
{
	Vec2f retStick;

	// Determine how far the controller is pushed
	float magnitude = sqrtf(stickX * stickX + stickY * stickY);
	float normalizedMagnitude = 0;

	// Chek if the controller is outside a circular dead zone
	if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		// Normalise the stick positions
		retStick.x = (float)(stickX) / magnitude;
		retStick.y = (float)(stickY) / magnitude;

		// Clamp the magnitude to the maximim expected value
		if (magnitude > 32767) magnitude = 32767;

		magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

		normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

		// Normalise the stick positions
		retStick.x *= normalizedMagnitude;
		retStick.y *= normalizedMagnitude;
	}
	else
	{
		retStick.x = 0;
		retStick.y = 0;
	}

	return retStick;
}