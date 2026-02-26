#pragma once
#include <functional>
#include <type_traits>
#include "UIHandle.h"

namespace std
{
	template <>
	struct hash<UIHandle>
	{
		std::size_t operator()(const UIHandle& e) const noexcept
		{
			return (static_cast<std::size_t>(e.id) << 32) | static_cast<std::size_t>(e.generation);
		}
	};
}