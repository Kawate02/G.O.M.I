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
			//大きめの素数を掛けて混ぜる(xor だけだと (1,2) と (2,1) がぶつかる)
			std::size_t h = static_cast<std::size_t>(static_cast<uint32_t>(cell.x)) * 73856093u;
			h ^= static_cast<std::size_t>(static_cast<uint32_t>(cell.y)) * 19349663u;
			h ^= static_cast<std::size_t>(static_cast<uint32_t>(cell.z)) * 83492791u;
			return h;
		}
	};
}
