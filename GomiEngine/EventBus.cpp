#include "EventBus.h"

ListenerHandle EventBus::subscribe(std::string eventName, Callback callback)
{
	listeners[eventName].push_back({ callback, true });
	return ListenerHandle{ eventName, listeners[eventName].size() - 1 };
}

void EventBus::unsubscribe(ListenerHandle handle)
{
	auto it = listeners.find(handle.eventName);
	if (it != listeners.end() && handle.index < it->second.size()) 
	{
		it->second[handle.index].active = false;
	}
}

void EventBus::cleanup()
{
	for (auto& pair : listeners)
	{
		auto& vec = pair.second;
		vec.erase(std::remove_if(vec.begin(), vec.end(), [](const ListenerEntry& entry) { return !entry.active; }), vec.end());
	}
}
