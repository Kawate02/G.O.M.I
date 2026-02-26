#include "DxInput.h"

struct KeyMap
{
	int key;
	InputKey inputKey;
};
constexpr KeyMap keyMap[] = {
	{ KEY_INPUT_BACK, InputKey::KEY_BACK },
	{ KEY_INPUT_TAB, InputKey::KEY_TAB },
	{ KEY_INPUT_RETURN, InputKey::KEY_RETURN },
	{ KEY_INPUT_LSHIFT, InputKey::KEY_LSHIFT },
	{ KEY_INPUT_RSHIFT, InputKey::KEY_RSHIFT },
	{ KEY_INPUT_LCONTROL, InputKey::KEY_LCONTROL },
	{ KEY_INPUT_RCONTROL, InputKey::KEY_RCONTROL },
	{ KEY_INPUT_ESCAPE, InputKey::KEY_ESCAPE },
	{ KEY_INPUT_SPACE, InputKey::KEY_SPACE },
	{ KEY_INPUT_PGUP, InputKey::KEY_PGUP },
	{ KEY_INPUT_PGDN, InputKey::KEY_PGDN },
	{ KEY_INPUT_END, InputKey::KEY_END },
	{ KEY_INPUT_HOME, InputKey::KEY_HOME },
	{ KEY_INPUT_LEFT, InputKey::KEY_LEFT },
	{ KEY_INPUT_UP, InputKey::KEY_UP },
	{ KEY_INPUT_RIGHT, InputKey::KEY_RIGHT },
	{ KEY_INPUT_DOWN, InputKey::KEY_DOWN },
	{ KEY_INPUT_INSERT, InputKey::KEY_INSERT },
	{ KEY_INPUT_DELETE, InputKey::KEY_DELETE },
	{ KEY_INPUT_MINUS, InputKey::KEY_MINUS },
	{ KEY_INPUT_YEN, InputKey::KEY_YEN },
	{ KEY_INPUT_PREVTRACK, InputKey::KEY_PREVTRACK },
	{ KEY_INPUT_PERIOD, InputKey::KEY_PERIOD },
	{ KEY_INPUT_SLASH, InputKey::KEY_SLASH },
	{ KEY_INPUT_NUMPAD0, InputKey::KEY_NUMPAD0 },
	{ KEY_INPUT_NUMPAD1, InputKey::KEY_NUMPAD1 },
	{ KEY_INPUT_NUMPAD2, InputKey::KEY_NUMPAD2 },
	{ KEY_INPUT_NUMPAD3, InputKey::KEY_NUMPAD3 },
	{ KEY_INPUT_NUMPAD4, InputKey::KEY_NUMPAD4 },
	{ KEY_INPUT_NUMPAD5, InputKey::KEY_NUMPAD5 },
	{ KEY_INPUT_NUMPAD6, InputKey::KEY_NUMPAD6 },
	{ KEY_INPUT_NUMPAD7, InputKey::KEY_NUMPAD7 },
	{ KEY_INPUT_NUMPAD8, InputKey::KEY_NUMPAD8 },
	{ KEY_INPUT_NUMPAD9, InputKey::KEY_NUMPAD9 },
	{ KEY_INPUT_MULTIPLY, InputKey::KEY_MULTIPLY },
	{ KEY_INPUT_ADD, InputKey::KEY_ADD },
	{ KEY_INPUT_SUBTRACT, InputKey::KEY_SUBTRACT },
	{ KEY_INPUT_DECIMAL, InputKey::KEY_DECIMAL },
	{ KEY_INPUT_DIVIDE, InputKey::KEY_DIVIDE },
	{ KEY_INPUT_F1, InputKey::KEY_F1 },
	{ KEY_INPUT_F2, InputKey::KEY_F2 },
	{ KEY_INPUT_F3, InputKey::KEY_F3 },
	{ KEY_INPUT_F4, InputKey::KEY_F4 },
	{ KEY_INPUT_F5, InputKey::KEY_F5 },
	{ KEY_INPUT_F6, InputKey::KEY_F6 },
	{ KEY_INPUT_F7, InputKey::KEY_F7 },
	{ KEY_INPUT_F8, InputKey::KEY_F8 },
	{ KEY_INPUT_F9, InputKey::KEY_F9 },
	{ KEY_INPUT_F10, InputKey::KEY_F10 },
	{ KEY_INPUT_F11, InputKey::KEY_F11 },
	{ KEY_INPUT_F12, InputKey::KEY_F12 },
	{ KEY_INPUT_A, InputKey::KEY_A },
	{ KEY_INPUT_B, InputKey::KEY_B },
	{ KEY_INPUT_C, InputKey::KEY_C },
	{ KEY_INPUT_D, InputKey::KEY_D },
	{ KEY_INPUT_E, InputKey::KEY_E },
	{ KEY_INPUT_F, InputKey::KEY_F },
	{ KEY_INPUT_G, InputKey::KEY_G },
	{ KEY_INPUT_H, InputKey::KEY_H },
	{ KEY_INPUT_I, InputKey::KEY_I },
	{ KEY_INPUT_J, InputKey::KEY_J },
	{ KEY_INPUT_K, InputKey::KEY_K },
	{ KEY_INPUT_L, InputKey::KEY_L },
	{ KEY_INPUT_M, InputKey::KEY_M },
	{ KEY_INPUT_N, InputKey::KEY_N },
	{ KEY_INPUT_O, InputKey::KEY_O },
	{ KEY_INPUT_P, InputKey::KEY_P },
	{ KEY_INPUT_Q, InputKey::KEY_Q },
	{ KEY_INPUT_R, InputKey::KEY_R },
	{ KEY_INPUT_S, InputKey::KEY_S },
	{ KEY_INPUT_T, InputKey::KEY_T },
	{ KEY_INPUT_U, InputKey::KEY_U },
	{ KEY_INPUT_V, InputKey::KEY_V },
	{ KEY_INPUT_W, InputKey::KEY_W },
	{ KEY_INPUT_X, InputKey::KEY_X },
	{ KEY_INPUT_Y, InputKey::KEY_Y },
	{ KEY_INPUT_0, InputKey::KEY_NUM_0 },
	{ KEY_INPUT_1, InputKey::KEY_NUM_1 },
	{ KEY_INPUT_2, InputKey::KEY_NUM_2 },
	{ KEY_INPUT_3, InputKey::KEY_NUM_3 },
	{ KEY_INPUT_4, InputKey::KEY_NUM_4 },
	{ KEY_INPUT_5, InputKey::KEY_NUM_5 },
	{ KEY_INPUT_6, InputKey::KEY_NUM_6 },
	{ KEY_INPUT_7, InputKey::KEY_NUM_7 },
	{ KEY_INPUT_8, InputKey::KEY_NUM_8 },
	{ KEY_INPUT_9, InputKey::KEY_NUM_9 },
};

struct mouseMap
{
	int key;
	InputKey inputKey;
};
constexpr mouseMap mButtonMap[] = {
	{ MOUSE_INPUT_LEFT, InputKey::MOUSE_LEFT },
	{ MOUSE_INPUT_RIGHT, InputKey::MOUSE_RIGHT },
	{ MOUSE_INPUT_MIDDLE, InputKey::MOUSE_MIDDLE },
	{ MOUSE_INPUT_4, InputKey::MOUSE_4 },
	{ MOUSE_INPUT_5, InputKey::MOUSE_5 },
};

void DxInput::convertMouseButton()
{
	int x, y;
	GetMousePoint(&x, &y);

	inputState.mouse.deltaPosition = inputState.mouse.position;
	inputState.mouse.position = { (float)x, (float)y };

	auto button = GetMouseInput();

	for (auto& m : mButtonMap)
	{
		inputState.deltakey[static_cast<int>(m.inputKey)] = inputState.key[static_cast<int>(m.inputKey)];
		inputState.key[static_cast<int>(m.inputKey)] = button & m.key;
	}

	inputState.mouse.deltaWheel = GetMouseWheelRotVol();
	inputState.mouse.totalWheel += inputState.mouse.deltaWheel;
}

void DxInput::convertAllKeycodes()
{
	char key[256];
	GetHitKeyStateAll(key);
	for (auto& m : keyMap)
	{
		inputState.deltakey[static_cast<int>(m.inputKey)] = inputState.key[static_cast<int>(m.inputKey)];
		inputState.key[static_cast<int>(m.inputKey)] = key[m.key] != 0;
	}
}

/*************  ✨ Windsurf Command ⭐  *************/
/*******  45ebab30-1593-49ea-87dc-4711f6678cd0  *******/
void DxInput::inputStateUpdate()
{
	convertAllKeycodes();
	convertMouseButton();
}

const InputState& DxInput::getInputState()
{
	return inputState;
}