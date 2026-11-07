#include "GlfwInput.h"
#include "GlfwWindow.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace
{
	struct KeyMap
	{
		int glfwKey;
		InputKey inputKey;
	};

	// GLFWのキーは米国配列の位置で決まる。日本語配列固有のキー(YEN・AT・COLON・PREVTRACK)は対応先が無いので割り当てない
	constexpr KeyMap keyMap[] = {
		{ GLFW_KEY_BACKSPACE, InputKey::KEY_BACK },
		{ GLFW_KEY_TAB, InputKey::KEY_TAB },
		{ GLFW_KEY_ENTER, InputKey::KEY_RETURN },
		{ GLFW_KEY_LEFT_SHIFT, InputKey::KEY_LSHIFT },
		{ GLFW_KEY_RIGHT_SHIFT, InputKey::KEY_RSHIFT },
		{ GLFW_KEY_LEFT_CONTROL, InputKey::KEY_LCONTROL },
		{ GLFW_KEY_RIGHT_CONTROL, InputKey::KEY_RCONTROL },
		{ GLFW_KEY_ESCAPE, InputKey::KEY_ESCAPE },
		{ GLFW_KEY_SPACE, InputKey::KEY_SPACE },
		{ GLFW_KEY_PAGE_UP, InputKey::KEY_PGUP },
		{ GLFW_KEY_PAGE_DOWN, InputKey::KEY_PGDN },
		{ GLFW_KEY_END, InputKey::KEY_END },
		{ GLFW_KEY_HOME, InputKey::KEY_HOME },
		{ GLFW_KEY_LEFT, InputKey::KEY_LEFT },
		{ GLFW_KEY_UP, InputKey::KEY_UP },
		{ GLFW_KEY_RIGHT, InputKey::KEY_RIGHT },
		{ GLFW_KEY_DOWN, InputKey::KEY_DOWN },
		{ GLFW_KEY_INSERT, InputKey::KEY_INSERT },
		{ GLFW_KEY_DELETE, InputKey::KEY_DELETE },
		{ GLFW_KEY_MINUS, InputKey::KEY_MINUS },
		{ GLFW_KEY_PERIOD, InputKey::KEY_PERIOD },
		{ GLFW_KEY_SLASH, InputKey::KEY_SLASH },
		{ GLFW_KEY_LEFT_ALT, InputKey::KEY_LALT },
		{ GLFW_KEY_RIGHT_ALT, InputKey::KEY_RALT },
		{ GLFW_KEY_SCROLL_LOCK, InputKey::KEY_SCROLL },
		{ GLFW_KEY_SEMICOLON, InputKey::KEY_SEMICOLON },
		{ GLFW_KEY_LEFT_BRACKET, InputKey::KEY_LBRACKET },
		{ GLFW_KEY_RIGHT_BRACKET, InputKey::KEY_RBRACKET },
		{ GLFW_KEY_BACKSLASH, InputKey::KEY_BACKSLASH },
		{ GLFW_KEY_COMMA, InputKey::KEY_COMMA },
		{ GLFW_KEY_CAPS_LOCK, InputKey::KEY_CAPSLOCK },
		{ GLFW_KEY_KP_0, InputKey::KEY_NUMPAD0 },
		{ GLFW_KEY_KP_1, InputKey::KEY_NUMPAD1 },
		{ GLFW_KEY_KP_2, InputKey::KEY_NUMPAD2 },
		{ GLFW_KEY_KP_3, InputKey::KEY_NUMPAD3 },
		{ GLFW_KEY_KP_4, InputKey::KEY_NUMPAD4 },
		{ GLFW_KEY_KP_5, InputKey::KEY_NUMPAD5 },
		{ GLFW_KEY_KP_6, InputKey::KEY_NUMPAD6 },
		{ GLFW_KEY_KP_7, InputKey::KEY_NUMPAD7 },
		{ GLFW_KEY_KP_8, InputKey::KEY_NUMPAD8 },
		{ GLFW_KEY_KP_9, InputKey::KEY_NUMPAD9 },
		{ GLFW_KEY_KP_MULTIPLY, InputKey::KEY_MULTIPLY },
		{ GLFW_KEY_KP_ADD, InputKey::KEY_ADD },
		{ GLFW_KEY_KP_SUBTRACT, InputKey::KEY_SUBTRACT },
		{ GLFW_KEY_KP_DECIMAL, InputKey::KEY_DECIMAL },
		{ GLFW_KEY_KP_DIVIDE, InputKey::KEY_DIVIDE },
		{ GLFW_KEY_KP_ENTER, InputKey::KEY_NUMPADENTER },
		{ GLFW_KEY_F1, InputKey::KEY_F1 },
		{ GLFW_KEY_F2, InputKey::KEY_F2 },
		{ GLFW_KEY_F3, InputKey::KEY_F3 },
		{ GLFW_KEY_F4, InputKey::KEY_F4 },
		{ GLFW_KEY_F5, InputKey::KEY_F5 },
		{ GLFW_KEY_F6, InputKey::KEY_F6 },
		{ GLFW_KEY_F7, InputKey::KEY_F7 },
		{ GLFW_KEY_F8, InputKey::KEY_F8 },
		{ GLFW_KEY_F9, InputKey::KEY_F9 },
		{ GLFW_KEY_F10, InputKey::KEY_F10 },
		{ GLFW_KEY_F11, InputKey::KEY_F11 },
		{ GLFW_KEY_F12, InputKey::KEY_F12 },
		{ GLFW_KEY_A, InputKey::KEY_A },
		{ GLFW_KEY_B, InputKey::KEY_B },
		{ GLFW_KEY_C, InputKey::KEY_C },
		{ GLFW_KEY_D, InputKey::KEY_D },
		{ GLFW_KEY_E, InputKey::KEY_E },
		{ GLFW_KEY_F, InputKey::KEY_F },
		{ GLFW_KEY_G, InputKey::KEY_G },
		{ GLFW_KEY_H, InputKey::KEY_H },
		{ GLFW_KEY_I, InputKey::KEY_I },
		{ GLFW_KEY_J, InputKey::KEY_J },
		{ GLFW_KEY_K, InputKey::KEY_K },
		{ GLFW_KEY_L, InputKey::KEY_L },
		{ GLFW_KEY_M, InputKey::KEY_M },
		{ GLFW_KEY_N, InputKey::KEY_N },
		{ GLFW_KEY_O, InputKey::KEY_O },
		{ GLFW_KEY_P, InputKey::KEY_P },
		{ GLFW_KEY_Q, InputKey::KEY_Q },
		{ GLFW_KEY_R, InputKey::KEY_R },
		{ GLFW_KEY_S, InputKey::KEY_S },
		{ GLFW_KEY_T, InputKey::KEY_T },
		{ GLFW_KEY_U, InputKey::KEY_U },
		{ GLFW_KEY_V, InputKey::KEY_V },
		{ GLFW_KEY_W, InputKey::KEY_W },
		{ GLFW_KEY_X, InputKey::KEY_X },
		{ GLFW_KEY_Y, InputKey::KEY_Y },
		{ GLFW_KEY_Z, InputKey::KEY_Z },
		{ GLFW_KEY_0, InputKey::KEY_NUM_0 },
		{ GLFW_KEY_1, InputKey::KEY_NUM_1 },
		{ GLFW_KEY_2, InputKey::KEY_NUM_2 },
		{ GLFW_KEY_3, InputKey::KEY_NUM_3 },
		{ GLFW_KEY_4, InputKey::KEY_NUM_4 },
		{ GLFW_KEY_5, InputKey::KEY_NUM_5 },
		{ GLFW_KEY_6, InputKey::KEY_NUM_6 },
		{ GLFW_KEY_7, InputKey::KEY_NUM_7 },
		{ GLFW_KEY_8, InputKey::KEY_NUM_8 },
		{ GLFW_KEY_9, InputKey::KEY_NUM_9 },
	};

	constexpr KeyMap mouseMap[] = {
		{ GLFW_MOUSE_BUTTON_LEFT, InputKey::MOUSE_LEFT },
		{ GLFW_MOUSE_BUTTON_RIGHT, InputKey::MOUSE_RIGHT },
		{ GLFW_MOUSE_BUTTON_MIDDLE, InputKey::MOUSE_MIDDLE },
		{ GLFW_MOUSE_BUTTON_4, InputKey::MOUSE_4 },
		{ GLFW_MOUSE_BUTTON_5, InputKey::MOUSE_5 },
	};
}

void GlfwInput::inputStateUpdate()
{
	GLFWwindow* w = window.handle();
	if (!w) return;

	for (auto& m : keyMap)
	{
		int i = static_cast<int>(m.inputKey);
		inputState.deltakey[i] = inputState.key[i];
		inputState.key[i] = glfwGetKey(w, m.glfwKey) == GLFW_PRESS;
	}
	for (auto& m : mouseMap)
	{
		int i = static_cast<int>(m.inputKey);
		inputState.deltakey[i] = inputState.key[i];
		inputState.key[i] = glfwGetMouseButton(w, m.glfwKey) == GLFW_PRESS;
	}

	double x, y;
	glfwGetCursorPos(w, &x, &y);
	Vec2 pos = { static_cast<float>(x), static_cast<float>(y) };
	inputState.mouse.deltaPosition = firstUpdate ? Vec2{ 0, 0 } : pos - inputState.mouse.position;
	inputState.mouse.position = pos;
	firstUpdate = false;

	inputState.mouse.deltaWheel = window.consumeScroll();
	inputState.mouse.totalWheel += inputState.mouse.deltaWheel;
}

const InputState& GlfwInput::getInputState()
{
	return inputState;
}
