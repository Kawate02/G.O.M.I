#pragma once
#include "Vec3.h"
#include <cmath>
#include <vector>

/// <summary>
/// 当たり判定の空間分割グリッドの1マス
/// </summary>
struct Cell
{
	int x = 0;
	int y = 0;
	int z = 0;

	inline bool operator==(const Cell& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

/// <summary>
/// min から size の箱が重なるマスを全部返す(厚さ 0 の軸も1マスとして数える)
/// </summary>
inline std::vector<Cell> toCell(const Vec3& pos, const Vec3& size, int cellSize)
{
	std::vector<Cell> cells;
	int x0 = static_cast<int>(std::floor(pos.x / cellSize));
	int x1 = static_cast<int>(std::floor((pos.x + size.x) / cellSize));
	int y0 = static_cast<int>(std::floor(pos.y / cellSize));
	int y1 = static_cast<int>(std::floor((pos.y + size.y) / cellSize));
	int z0 = static_cast<int>(std::floor(pos.z / cellSize));
	int z1 = static_cast<int>(std::floor((pos.z + size.z) / cellSize));
	for (int z = z0; z <= z1; z++)
		for (int y = y0; y <= y1; y++)
			for (int x = x0; x <= x1; x++)
				cells.push_back({ x, y, z });
	return cells;
}
