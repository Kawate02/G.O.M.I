#pragma once

class IAudioBackend
{
public:
	virtual void play(int handle, float volume) = 0;
	virtual bool isPlaying(int handle) = 0;
	virtual void destroy(int handle) = 0;
};