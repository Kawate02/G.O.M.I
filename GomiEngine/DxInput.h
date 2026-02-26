#pragma once
#include "IInput.h"
#include <DxLib.h>

class DxInput : public IInput
{
private:
	void convertMouseButton();
	/// <summary>
	/// DXライブラリのキーコードを自前のものに変換
	/// </summary>
	/// <param name="keyCode"></param>
	/// <returns></returns>
	void convertAllKeycodes();
public:
	/// <summary>
	/// DXライブラリに入力を見てもらう
	/// </summary>
	void inputStateUpdate() override;
	const InputState& getInputState() override;
};