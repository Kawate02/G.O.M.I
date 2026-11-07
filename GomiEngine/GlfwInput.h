#pragma once
#include "IInput.h"

class GlfwWindow;

class GlfwInput : public IInput
{
private:
	GlfwWindow& window;
	bool firstUpdate = true;
public:
	explicit GlfwInput(GlfwWindow& window) : window(window) {}
	/// <summary>
	/// GLFWからキー・マウスの状態を読む
	/// </summary>
	void inputStateUpdate() override;
	const InputState& getInputState() override;
};
