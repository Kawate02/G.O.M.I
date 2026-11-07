#pragma once
#include "AudioQueue.h"
#include "ResourceManager.h"
#include "IAudioBackend.h"
#include "EventRequest.h"

struct BGMID
{
	std::string bgmID;
	int volume = 0;
};

class AudioPlayer
{
private:
	EventRequest<BGMID> bgmChangeRequest{ "BGMChange" };
	int bgmSound = -1;
	int bgmVoice = -1;
	int bgmVolume = 0;
public:
	void play(IAudioBackend& backend, AudioQueue& queue);
};
