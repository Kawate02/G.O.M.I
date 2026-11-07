#pragma once
#include "IWindow.h"

struct GLFWwindow;

class GlfwWindow : public IWindow
{
private:
	GLFWwindow* window = nullptr;
	int width = 0;
	int height = 0;
	double scrollAccum = 0.0;
	static void onScroll(GLFWwindow* w, double xoffset, double yoffset);
public:
	~GlfwWindow() override;
	void init(int width, int height, const char* title, bool fullscreen = false) override;
	bool processMessage() override;

	inline GLFWwindow* handle() const { return window; }
	/// <summary>
	/// ゲーム側が想定している画面の大きさ(論理サイズ)
	/// </summary>
	inline int getWidth() const { return width; }
	inline int getHeight() const { return height; }
	/// <summary>
	/// 前回呼んでから回ったホイールの量(ノッチ数)
	/// </summary>
	int consumeScroll();
};
