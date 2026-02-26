#pragma once
#include "Utilities.h"
#include "EntityHash.h"
#include "MoveEvent.h"
#include "CollisionEvent.h"

class WorldEventBus
{
private:
	std::unordered_map<Entity, std::vector<MoveRequestEvent>> moveEvents;
	std::unordered_map<Entity, std::vector<CollisionEvent>> collisionEvents;
public:
    template<typename T>
    void emit(T event)
    {
		if constexpr (std::is_same_v<T, MoveRequestEvent>)
		{
			moveEvents[event.e].push_back(event);
		}
        else if constexpr (std::is_same_v<T, CollisionEvent>)
		{
			collisionEvents[event.e].push_back(event);
		}
    }

    template<typename T>
    std::unordered_map<Entity, std::vector<T>>& getQueue()
    {
		if constexpr (std::is_same_v<T, MoveRequestEvent>)
		{
			return moveEvents;
		}
		else if constexpr (std::is_same_v<T, CollisionEvent>)
		{
			return collisionEvents;
		}
    }
    inline void clearEvents()
    { 
        moveEvents.clear(); 
		collisionEvents.clear();
    }
};