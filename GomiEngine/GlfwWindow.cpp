#include "GlfwWindow.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

GlfwWindow::~GlfwWindow()
{
	if (window) glfwDestroyWindow(window);
	glfwTerminate();
#ifdef _WIN32
	timeEndPeriod(1);
#endif
}

void GlfwWindow::init(int width, int height, const char* title, bool fullscreen)
{
#ifdef _WIN32
	// フレーム待ちの sleep を1ms単位で効かせる
	timeBeginPeriod(1);
#endif
	glfwSetErrorCallback([](int code, const char* desc) { std::cerr << "GLFW error " << code << ": " << desc << std::endl; });
	if (!glfwInit()) throw std::runtime_error("glfwInit failed");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->width = width;
	this->height = height;
	window = glfwCreateWindow(width, height, title, fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
	if (!window) throw std::runtime_error("glfwCreateWindow failed (OpenGL 4.5 is required)");

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0); // フレームレートは Game 側で合わせる
	glfwSetWindowUserPointer(window, this);
	glfwSetScrollCallback(window, &GlfwWindow::onScroll);
}

bool GlfwWindow::processMessage()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(window);
}

void GlfwWindow::onScroll(GLFWwindow* w, double xoffset, double yoffset)
{
	auto self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
	if (self) self->scrollAccum += yoffset;
}

int GlfwWindow::consumeScroll()
{
	int notches = static_cast<int>(std::trunc(scrollAccum));
	scrollAccum -= notches;
	return notches;
}
