#pragma once
#include <functional>
#include <type_traits>
#include "Entity.h"

namespace std
{
	template <>
	struct hash<Entity>
	{
		std::size_t operator()(const Entity& e) const noexcept
		{
			return (static_cast<std::size_t>(e.id) << 32) | static_cast<std::size_t>(e.generation);
		}
	};
}