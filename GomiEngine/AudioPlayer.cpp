#include "AudioPlayer.h"

void AudioPlayer::play(IAudioBackend& backend, AudioQueue& queue)
{
	BGMID nextBGM;
	if (bgmChangeRequest.trigger(nextBGM))
	{
		if (bgmHandle != -1)
		{
			backend.destroy(bgmHandle);
		}
		auto res = ResourceManager::getInstance().loadSound(nextBGM.bgmID);
		bgmHandle = res.handle;
		bgmVolume = nextBGM.volume;
	}
	if (bgmHandle != -1)
	{
		if (!backend.isPlaying(bgmHandle))
		{
			backend.play(bgmHandle, bgmVolume);
		}
	}
	auto items = queue.getCommands();
	queue.clear();
	for (auto& item : items)
	{
		auto res = ResourceManager::getInstance().loadSound(item.audioID);
		if (res.handle == -1) continue;
		backend.play(res.handle, item.volume);
	}
}

void AudioPlayer::destroy()
{

}
