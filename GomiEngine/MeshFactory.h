#pragma once
#include "IDrawer.h"
#include <optional>
#include <string>

/// <summary>
/// ファイル無しで使える基本形。アセット ID は "builtin:cube" のように書く。
/// どれも原点中心で大きさ 1(cube は1辺1、sphere は直径1、plane は XZ 平面で1x1・上向き)
/// </summary>
namespace MeshFactory
{
	MeshData cube();
	MeshData plane();
	MeshData sphere(int segments = 24, int rings = 16);
	/// <summary>
	/// "builtin:..." なら作って返す。それ以外・知らない名前は nullopt
	/// </summary>
	std::optional<MeshData> builtin(const std::string& id);
	/// <summary>
	/// 法線の無いメッシュに、面の向きから法線を付ける
	/// </summary>
	void computeNormals(MeshData& mesh);
}
