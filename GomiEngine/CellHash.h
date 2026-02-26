#pragma once
#include <functional>
#include <type_traits>
#include "Cell.h"

namespace std
{
	template<>
	struct hash<Cell>
	{
		std::size_t operator()(const Cell& cell) const
		{
			return std::hash<int>()(cell.x) ^ std::hash<int>()(cell.y);
		}
	};
}