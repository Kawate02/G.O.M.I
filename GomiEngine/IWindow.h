#pragma once

/// <summary>
/// ウィンドウの抽象レイヤー
/// </summary>
class IWindow
{
public:
    virtual void init(int width, int height, const char* title, bool fullscreen = false) = 0;
    /// <summary>
    /// OSのメッセージを処理する。ウィンドウが閉じられたら false
    /// </summary>
    virtual bool processMessage() = 0;
    virtual ~IWindow() = default;
};
