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
	Callback callback;
	bool active = true;
};

struct ListenerHandle 
{
	std::string eventName;
	size_t index;
};

class EventBus
{
private:
	EventBus() = default;
	std::unordered_map<std::string, std::vector<ListenerEntry>> listeners;
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
			for (auto& entry : it->second)
			{
				if (entry.active) entry.callback(data);
			}
		}
	}
	void cleanup();
};