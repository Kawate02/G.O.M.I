#pragma once
#include "IAudioBackend.h"
#include <memory>

/// <summary>
/// miniaudio による音声。同じ音を重ねて鳴らせるように、鳴らすたびにボイスを作る
/// </summary>
class MiniaudioBackend : public IAudioBackend
{
private:
	struct Impl;
	std::unique_ptr<Impl> impl;
public:
	MiniaudioBackend();
	~MiniaudioBackend() override;
	void init() override;
	int load(const std::string& name, const void* data, size_t size) override;
	int play(int sound, float volume) override;
	bool isPlaying(int voice) override;
	void stop(int voice) override;
	void update() override;
};
