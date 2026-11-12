#include "EventBus.h"

ListenerHandle EventBus::subscribe(std::string eventName, Callback callback)
{
	size_t id = nextId++;
	listeners[eventName].push_back({ id, callback, true });
	return ListenerHandle{ eventName, id };
}

void EventBus::unsubscribe(ListenerHandle handle)
{
	auto it = listeners.find(handle.eventName);
	if (it == listeners.end()) return;
	for (auto& entry : it->second)
	{
		if (entry.id == handle.id) entry.active = false;
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

EventBus& EventBus::getInstance()
{
	static EventBus instance;
	return instance;
}
