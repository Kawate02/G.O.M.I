#pragma once

class IWindow 
{
public:
    virtual void init(int width, int height, const char* title, bool fullscreen = false) = 0;
    virtual bool processMessage() = 0;
    virtual ~IWindow() = default;
};