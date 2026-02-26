#pragma once
#include "IWindow.h"
#include <DxLib.h>

class DxWindow : public IWindow 
{
public:
    ~DxWindow() { DxLib_End(); }

    void init(int width, int height, const char* title, bool fullscreen = false) override;
    bool processMessage() override 
    {
        return (ProcessMessage() == 0);
    }
};