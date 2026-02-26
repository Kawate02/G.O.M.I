#include "DxAudioBackend.h"

void DxAudioBackend::play(int handle, float volume)
{
	ChangeVolumeSoundMem(volume, handle);
	PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

bool DxAudioBackend::isPlaying(int handle)
{
	return CheckSoundMem(handle) == 1;
}

void DxAudioBackend::destroy(int handle)
{
	DeleteSoundMem(handle);
}
