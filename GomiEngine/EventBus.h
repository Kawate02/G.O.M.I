#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <any>

#include "Utilities.h"

namespace Event
{
	const static std::string Pause_Close = "PauseClose";
	const static std::string Pause_Open = "PauseOpen";
	const static std::string World_Change = "WorldChange";
	const static std::string Game_Quit = "quit";
	const static std::string Audio_Play = "audioPlay";
};

using Callback = std::function<void(std::any)>;

struct ListenerEntry
{
	size_t id;
	Callback callback;
	bool active = true;
};

struct ListenerHandle 
{
	std::string eventName;
	size_t id = 0;
};

class EventBus
{
private:
	EventBus() = default;
	std::unordered_map<std::string, std::vector<ListenerEntry>> listeners;
	size_t nextId = 1;
public:
	static EventBus& getInstance()
	{
		static EventBus instance;
		return instance;
	}

	ListenerHandle subscribe(std::string eventName, Callback callback);
	void unsubscribe(ListenerHandle handle);
	template<typename T>
	void emit(std::string eventName, T data)
	{
		auto it = listeners.find(eventName);
		if (it != listeners.end())
		{
			//呼び出し中に購読が増えても壊れないよう、添字で回して増えた分は次回から
			auto& vec = it->second;
			const size_t n = vec.size();
			for (size_t i = 0; i < n; i++)
			{
				if (vec[i].active) vec[i].callback(data);
			}
		}
	}
	/// <summary>
	/// 購読解除されたものを取り除く。emit の外(フレームの区切り)で呼ぶ
	/// </summary>
	void cleanup();
};