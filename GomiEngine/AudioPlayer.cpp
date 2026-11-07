#include "AudioPlayer.h"

void AudioPlayer::play(IAudioBackend& backend, AudioQueue& queue)
{
	BGMID nextBGM;
	if (bgmChangeRequest.trigger(nextBGM))
	{
		if (bgmVoice != -1)
		{
			backend.stop(bgmVoice);
			bgmVoice = -1;
		}
		bgmSound = ResourceManager::getInstance().loadSound(backend, nextBGM.bgmID).handle;
		bgmVolume = nextBGM.volume;
	}
	// BGM は鳴り終わったら最初から鳴らし直す
	if (bgmSound != -1 && (bgmVoice == -1 || !backend.isPlaying(bgmVoice)))
	{
		bgmVoice = backend.play(bgmSound, static_cast<float>(bgmVolume));
	}
	auto items = queue.getCommands();
	queue.clear();
	for (auto& item : items)
	{
		auto res = ResourceManager::getInstance().loadSound(backend, item.audioID);
		if (res.handle == -1) continue;
		backend.play(res.handle, item.volume);
	}
	backend.update();
}
