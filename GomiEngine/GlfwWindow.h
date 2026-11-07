#pragma once
#include "IWindow.h"
#include <string>

struct GLFWwindow;

class GlfwWindow : public IWindow
{
private:
	GLFWwindow* window = nullptr;
	int width = 0;
	int height = 0;
	double scrollAccum = 0.0;
	bool resizable = false;
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
	/// <summary>
	/// init の前に呼ぶと、マウスで大きさを変えられる窓になる(エディタ用)
	/// </summary>
	inline void setResizable(bool value) { resizable = value; }
	/// <summary>
	/// 閉じるボタンが押されたのを取り消す(保存の確認を出す時)
	/// </summary>
	void cancelClose();
	bool closeRequested() const;
	void setTitle(const std::string& title);
};
