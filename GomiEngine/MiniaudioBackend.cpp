#include "MiniaudioBackend.h"

#include <miniaudio.h>
#include <iostream>
#include <unordered_map>
#include <vector>

struct MiniaudioBackend::Impl
{
	ma_engine engine;
	bool ready = false;
	std::vector<std::string> sounds; // 音のハンドル = 添字。中身は resource manager に登録した名前
	std::unordered_map<int, std::unique_ptr<ma_sound>> voices;
	int nextVoice = 0;
};

MiniaudioBackend::MiniaudioBackend() : impl(std::make_unique<Impl>()) {}

MiniaudioBackend::~MiniaudioBackend()
{
	if (!impl->ready) return;
	for (auto& [id, voice] : impl->voices) ma_sound_uninit(voice.get());
	impl->voices.clear();
	ma_engine_uninit(&impl->engine);
}

void MiniaudioBackend::init()
{
	if (ma_engine_init(nullptr, &impl->engine) != MA_SUCCESS)
	{
		std::cerr << "audio device init failed: sound is disabled" << std::endl;
		return;
	}
	impl->ready = true;
}

int MiniaudioBackend::load(const std::string& name, const void* data, size_t size)
{
	if (!impl->ready || data == nullptr) return -1;
	auto* rm = ma_engine_get_resource_manager(&impl->engine);
	if (ma_resource_manager_register_encoded_data(rm, name.c_str(), data, size) != MA_SUCCESS) return -1;
	impl->sounds.push_back(name);
	return static_cast<int>(impl->sounds.size() - 1);
}

int MiniaudioBackend::play(int sound, float volume)
{
	if (!impl->ready || sound < 0 || sound >= static_cast<int>(impl->sounds.size())) return -1;
	auto voice = std::make_unique<ma_sound>();
	if (ma_sound_init_from_file(&impl->engine, impl->sounds[sound].c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, voice.get()) != MA_SUCCESS) return -1;
	ma_sound_set_volume(voice.get(), volume / 255.0f);
	ma_sound_start(voice.get());
	int id = impl->nextVoice++;
	impl->voices[id] = std::move(voice);
	return id;
}

bool MiniaudioBackend::isPlaying(int voice)
{
	auto it = impl->voices.find(voice);
	if (it == impl->voices.end()) return false;
	return !ma_sound_at_end(it->second.get());
}

void MiniaudioBackend::stop(int voice)
{
	auto it = impl->voices.find(voice);
	if (it == impl->voices.end()) return;
	ma_sound_uninit(it->second.get());
	impl->voices.erase(it);
}

void MiniaudioBackend::update()
{
	for (auto it = impl->voices.begin(); it != impl->voices.end();)
	{
		if (ma_sound_at_end(it->second.get()))
		{
			ma_sound_uninit(it->second.get());
			it = impl->voices.erase(it);
		}
		else ++it;
	}
}
