#pragma once

#include "Utilities.h"
#include "Entity.h"
#include "ComponentArray.h"
#include "ComponentType.h"

#include <unordered_map>
#include <typeindex>
#include <memory>

class ComponentManager
{
private:
	struct IComponentArray
	{
		virtual ~IComponentArray() = default;
		virtual void releaseAll() = 0;
	};

	template <typename T>
	struct ComponentArrayHolder : IComponentArray
	{
		ComponentArray<T> array;
		void releaseAll() override { array.releaseAll(); }
	};

	std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> arrays;

	template <typename T>
	ComponentArray<T>& getOrCreate()
	{
		auto& slot = arrays[std::type_index(typeid(T))];
		if (!slot)
		{
			slot = std::make_unique<ComponentArrayHolder<T>>();
		}
		return static_cast<ComponentArrayHolder<T>&>(*slot).array;
	}
public:
	template <typename T>
	void addComponent(Entity e, T component)
	{
		getOrCreate<T>().addComponent(e, component);
	}

	template <typename T>
	ComponentArray<T>& getComponentArray()
	{
		return getOrCreate<T>();
	}

	void releaseAll()
	{
		for (auto& [type, array] : arrays)
		{
			array->releaseAll();
		}
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