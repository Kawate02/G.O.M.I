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
	std::vector<CollisionEvent> collisionList; //出た順(毎回同じ順で回すため)
	inline static const std::vector<CollisionEvent> noCollisions;
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
			collisionEvents[event.self].push_back(event);
			collisionList.push_back(std::move(event));
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
	/// <summary>
	/// このフレームの当たり判定のイベント全部(出た順。ルールID → Entity の ID の順)
	/// </summary>
	inline const std::vector<CollisionEvent>& collisions() const { return collisionList; }
	/// <summary>
	/// e が受け取った当たり判定のイベント(出た順)
	/// </summary>
	inline const std::vector<CollisionEvent>& collisionsOf(Entity e) const
	{
		auto it = collisionEvents.find(e);
		return it == collisionEvents.end() ? noCollisions : it->second;
	}
    inline void clearEvents()
    {
        moveEvents.clear();
		collisionEvents.clear();
		collisionList.clear();
    }
};
