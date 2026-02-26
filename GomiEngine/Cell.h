#pragma once
#include "Vec2.h"
#include <cmath>
#include <vector>

struct Cell
{
	int x = 0;
	int y = 0;

	inline bool operator==(const Cell& other) const
	{
		return x == other.x && y == other.y;
	}
};

inline std::vector<Cell> toCell(const Vec2& pos, const Vec2& size, int cellSize)
{
	std::vector<Cell> cells;
	int left = static_cast<int>(std::floor(pos.x / cellSize));
	int right = static_cast<int>(std::ceil((pos.x + size.x) / cellSize));
	int top = static_cast<int>(std::floor(pos.y / cellSize));
	int bottom = static_cast<int>(std::ceil((pos.y + size.y) / cellSize));
	for (int y = top; y < bottom; y++)
		for (int x = left; x < right; x++)
			cells.push_back({ x, y });
	return cells;
}