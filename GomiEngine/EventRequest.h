#pragma once
#include <string>
#include "EventBus.h"

template<typename T>
class EventRequest
{
private:
	ListenerHandle handle;
	bool hasEvent = false;
	T value = T();
public:
	EventRequest(std::string eventName)
	{
		handle = EventBus::getInstance().subscribe(eventName, [this](std::any v) { setEvent(std::any_cast<const T&>(v)); });
	}
	~EventRequest()
	{
		EventBus::getInstance().unsubscribe(handle);
	}
	inline void setEvent(const T& v)
	{
		hasEvent = true;
		value = v;
	}

	inline bool trigger() 
	{ 
		if (hasEvent)
		{
			hasEvent = false;
			return true;
		}
		return false;
	}
	inline bool trigger(T& v)
	{
		if (hasEvent)
		{
			v = value;
			hasEvent = false;
			return true;
		}
		return false;
	}
	inline bool peek(T& v) const
	{
		if (hasEvent)
		{
			v = value;
			return true;
		}
		return false;
	}
};