#pragma once
#include "EnumNames.h"

// 論理キーの一覧。並び順がそのまま値になる(KEY_BACK = 0)
#define GOMI_INPUT_KEYS(X) \
	X(KEY_BACK) \
	X(KEY_TAB) \
	X(KEY_RETURN) \
	X(KEY_LSHIFT) \
	X(KEY_RSHIFT) \
	X(KEY_LCONTROL) \
	X(KEY_RCONTROL) \
	X(KEY_ESCAPE) \
	X(KEY_SPACE) \
	X(KEY_PGUP) \
	X(KEY_PGDN) \
	X(KEY_END) \
	X(KEY_HOME) \
	X(KEY_LEFT) \
	X(KEY_UP) \
	X(KEY_RIGHT) \
	X(KEY_DOWN) \
	X(KEY_INSERT) \
	X(KEY_DELETE) \
	X(KEY_MINUS) \
	X(KEY_YEN) \
	X(KEY_PREVTRACK) \
	X(KEY_PERIOD) \
	X(KEY_SLASH) \
	X(KEY_LALT) \
	X(KEY_RALT) \
	X(KEY_SCROLL) \
	X(KEY_SEMICOLON) \
	X(KEY_COLON) \
	X(KEY_LBRACKET) \
	X(KEY_RBRACKET) \
	X(KEY_AT) \
	X(KEY_BACKSLASH) \
	X(KEY_COMMA) \
	X(KEY_CAPSLOCK) \
	X(KEY_NUMPAD0) \
	X(KEY_NUMPAD1) \
	X(KEY_NUMPAD2) \
	X(KEY_NUMPAD3) \
	X(KEY_NUMPAD4) \
	X(KEY_NUMPAD5) \
	X(KEY_NUMPAD6) \
	X(KEY_NUMPAD7) \
	X(KEY_NUMPAD8) \
	X(KEY_NUMPAD9) \
	X(KEY_MULTIPLY) \
	X(KEY_ADD) \
	X(KEY_SUBTRACT) \
	X(KEY_DECIMAL) \
	X(KEY_DIVIDE) \
	X(KEY_NUMPADENTER) \
	X(KEY_F1) \
	X(KEY_F2) \
	X(KEY_F3) \
	X(KEY_F4) \
	X(KEY_F5) \
	X(KEY_F6) \
	X(KEY_F7) \
	X(KEY_F8) \
	X(KEY_F9) \
	X(KEY_F10) \
	X(KEY_F11) \
	X(KEY_F12) \
	X(KEY_A) \
	X(KEY_B) \
	X(KEY_C) \
	X(KEY_D) \
	X(KEY_E) \
	X(KEY_F) \
	X(KEY_G) \
	X(KEY_H) \
	X(KEY_I) \
	X(KEY_J) \
	X(KEY_K) \
	X(KEY_L) \
	X(KEY_M) \
	X(KEY_N) \
	X(KEY_O) \
	X(KEY_P) \
	X(KEY_Q) \
	X(KEY_R) \
	X(KEY_S) \
	X(KEY_T) \
	X(KEY_U) \
	X(KEY_V) \
	X(KEY_W) \
	X(KEY_X) \
	X(KEY_Y) \
	X(KEY_Z) \
	X(KEY_NUM_0) \
	X(KEY_NUM_1) \
	X(KEY_NUM_2) \
	X(KEY_NUM_3) \
	X(KEY_NUM_4) \
	X(KEY_NUM_5) \
	X(KEY_NUM_6) \
	X(KEY_NUM_7) \
	X(KEY_NUM_8) \
	X(KEY_NUM_9) \
	X(MOUSE_LEFT) \
	X(MOUSE_RIGHT) \
	X(MOUSE_MIDDLE) \
	X(MOUSE_4) \
	X(MOUSE_5)

enum class InputKey : int
{
#define GOMI_KEY_ENUM(name) name,
	GOMI_INPUT_KEYS(GOMI_KEY_ENUM)
#undef GOMI_KEY_ENUM
};

template <> struct EnumNames<InputKey>
{
	static const std::vector<std::pair<InputKey, const char*>>& list()
	{
#define GOMI_KEY_NAME(name) { InputKey::name, #name },
		static const std::vector<std::pair<InputKey, const char*>> entries = { GOMI_INPUT_KEYS(GOMI_KEY_NAME) };
#undef GOMI_KEY_NAME
		return entries;
	}
};
