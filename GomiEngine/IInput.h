#pragma once
#include "InputState.h"

/// <summary>
/// 入力システムの抽象レイヤー
/// </summary>
class IInput 
{
protected:
    InputState inputState;
public:
    virtual void inputStateUpdate() = 0;
    virtual const InputState& getInputState() = 0;
};