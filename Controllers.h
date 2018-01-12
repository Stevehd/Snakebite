//
//  Controllers.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 18/12/2016.
//  Copyright © 2016 Stephen Harrison-Daly. All rights reserved.
//

#pragma once

#include "Common.h"

#define SHD_MAX_CONTROLLERS				4
#define SHD_CONTROLLERS_NETWORK_INDEX	16
#define SHD_CONTROLLERS_KEYBOARD_INDEX	32
#define SHD_CONTROLLERS_INVALID_INDEX	-1

namespace shd
{
	class Controllers
	{
	public:

		enum KeyboardAction
		{
			KEYBOARD_ACTION_KEY_DOWN,
			KEYBOARD_ACTION_KEY_UP
		};

		enum ButtonType
		{
			BUTTON_UP = 0,
			BUTTON_DOWN,
			BUTTON_LEFT,
			BUTTON_RIGHT,
			BUTTON_START,
			BUTTON_SELECT,
			BUTTON_X,
			BUTTON_CIRCLE,
			BUTTON_SQUARE,
			BUTTON_TRIANGLE,
			BUTTON_L1,
			BUTTON_R1,
			BUTTON_L2,
			BUTTON_R2,
			BUTTON_L3,
			BUTTON_R3,
			BUTTON_MOUSE_LEFT,
			BUTTON_MOUSE_RIGHT,
			BUTTON_MAX
		};

		struct State
		{
			bool buttons[BUTTON_MAX];
			Vec2f leftStick;
			Vec2f rightStick;
			int16_t leftStickXInt;
			int16_t leftStickYInt;
			int16_t rightStickXInt;
			int16_t rightStickYInt;
			float leftTrigger;
			float rightTrigger;
			float rumbleStrengthBig;
			float rumbleStrengthSmall;
			bool isConnected;

			State();
			bool operator==(const State& rhs);
			bool operator!=(const State& rhs);
		};

		Controllers() : m_mousePosPrevSet(false), m_isAnyKeyboardKeyPressed(false) {}
		bool init();
		void update();
		void refreshControllerList();
		State & getControllerState(int controllerIndex);
		State & getControllerStatePrevious(int controllerIndex);
		bool isButtonPressed(int controllerIndex, ButtonType button);
		bool isButtonHeld(int controllerIndex, ButtonType button);
		bool isButtonReleased(int controllerIndex, ButtonType button);
		void setMousePos(int x, int y);
		void setMouseAsRightStick(Vec2f & stickPos);
		Vec2f getMousePos();
		Vec2f getMousePosUI();
		bool hasMousePosChanged();
		bool isConnected(int controllerIndex);
		static Vec2f sticksToVec2f(int16_t stickX, int16_t stickY);
		void setAnyKeyoboardKeyPressed(bool isPressed) { m_isAnyKeyboardKeyPressed = isPressed; }
		bool isAnyKeyoboardKeyPressed() { return m_isAnyKeyboardKeyPressed; }

	private:

		// This is used for game controllers
		State m_controllers[SHD_MAX_CONTROLLERS];

		// This is the game controller state in the previos framce
		State m_controllersPrevious[SHD_MAX_CONTROLLERS];

		// The same state, wrangled from keyboard inputs
		State m_keyboard;

		// State of the keyboard in the previous frame
		State m_keyboardPrevious;

		// State that was received over the network
		State m_networkController;

		// State of the network controller in the previous frame
		State m_networkControllerPrevious;

		// The mouse position
		Vec2f m_mousePos;

		// The previous mouse position
		Vec2f m_mousePosPrevious;

		// Has the previous mouse position been initially set
		// This was added because of a bug in the menus where the mouse kept an item highlighted if the user never moved the mouse
		bool m_mousePosPrevSet;

		// Used for the "press any button" screen 
		bool m_isAnyKeyboardKeyPressed;
	};
}