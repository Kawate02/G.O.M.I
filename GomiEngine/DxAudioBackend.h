#pragma once
#include "IAudioBackend.h"
#include <DxLib.h>

class DxAudioBackend : public IAudioBackend
{
public:
	void play(int handle, float volume) override;
	bool isPlaying(int handle) override;
	void destroy(int handle) override;
};