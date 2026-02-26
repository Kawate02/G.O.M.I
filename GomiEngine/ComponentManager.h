#pragma once

#include "Utilities.h"
#include "Entity.h"
#include "ComponentArray.h"
#include "ComponentType.h"

class ComponentManager
{
private:
	ComponentArray<TransformComponent> transformComponents;
	ComponentArray<PlayerControlComponent> playerInputComponents;
	ComponentArray<RendererComponent> rendererComponents;
	ComponentArray<MoveComponent> moveComponents;
	ComponentArray<PhysicsComponent> physicsComponents;
	ComponentArray<FowardMoveComponent> forwardMoveComponents;
	ComponentArray<CameraFollowComponent> cameraFollowComponents;
	ComponentArray<CollisionComponent> collisionComponents;
	ComponentArray<EntityTagComponent> entityTagComponents;
	ComponentArray<BackGroundComponent> backGroundComponents;
public:
	template <typename T>
	void addComponent(Entity e, T component)
	{
		if constexpr (std::is_same_v<T, TransformComponent>)
		{
			transformComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, PlayerControlComponent>)
		{
			playerInputComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, RendererComponent>)
		{
			rendererComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, MoveComponent>)
		{
			moveComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, PhysicsComponent>)
		{
			physicsComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, FowardMoveComponent>)
		{
			forwardMoveComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, CameraFollowComponent>)
		{
			cameraFollowComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, CollisionComponent>)
		{
			collisionComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, EntityTagComponent>)
		{
			entityTagComponents.addComponent(e, component);
		}
		else if constexpr (std::is_same_v<T, BackGroundComponent>)
		{
			backGroundComponents.addComponent(e, component);
		}
	}

	template <typename T>
	ComponentArray<T>& getComponentArray()
	{
		if constexpr (std::is_same_v<T, TransformComponent>)
		{
			return transformComponents;
		}
		else if constexpr (std::is_same_v<T, PlayerControlComponent>)
		{
			return playerInputComponents;
		}
		else if constexpr (std::is_same_v<T, RendererComponent>)
		{
			return rendererComponents;
		}
		else if constexpr (std::is_same_v<T, MoveComponent>)
		{
			return moveComponents;
		}
		else if constexpr (std::is_same_v<T, PhysicsComponent>)
		{
			return physicsComponents;
		}
		else if constexpr (std::is_same_v<T, FowardMoveComponent>)
		{
			return forwardMoveComponents;
		}
		else if constexpr (std::is_same_v<T, CameraFollowComponent>)
		{
			return cameraFollowComponents;
		}
		else if constexpr (std::is_same_v<T, CollisionComponent>)
		{
			return collisionComponents;
		}
		else if constexpr (std::is_same_v<T, EntityTagComponent>)
		{
			return entityTagComponents;
		}
		else if constexpr (std::is_same_v<T, BackGroundComponent>)
		{
			return backGroundComponents;
		}
	}
	void releaseAll()
	{
		transformComponents.releaseAll();
		playerInputComponents.releaseAll();
		rendererComponents.releaseAll();
		moveComponents.releaseAll();
		physicsComponents.releaseAll();
		forwardMoveComponents.releaseAll();
		cameraFollowComponents.releaseAll();
		collisionComponents.releaseAll();
		entityTagComponents.releaseAll();
		backGroundComponents.releaseAll();
	}

	template <typename T>
	bool has(Entity e) { return getComponentArray<T>().hasComponent(e); }

	template <typename... Ts, typename Func>
	void each(Func func) 
	{ 
		auto& first = getComponentArray<typename std::tuple_element<0, std::tuple<Ts...>>::type>();
		for (int i = 0; i < first.getComponentCount(); i++)
		{
			Entity e = { first.getEntityId(i), first.getGeneration(i) };
			
			if ((has<Ts>(e) && ...))
			{
				func(e);
			}
		}
	}
};