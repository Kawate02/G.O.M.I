// 基本形メッシュと glTF 読み込みのテスト。失敗したら行番号を出して 1 を返す
#include "Project.h"
#include "ModelLoader.h"
#include "MeshFactory.h"
#include "AssetDatabase.h"
#include "MathUtil.h"
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

struct Bounds
{
	glm::vec3 min{ 1e9f }, max{ -1e9f };
};

static Bounds bounds(const MeshData& m)
{
	Bounds b;
	for (size_t i = 0; i < m.vertices.size(); i += MeshData::STRIDE)
	{
		glm::vec3 p(m.vertices[i], m.vertices[i + 1], m.vertices[i + 2]);
		b.min = glm::min(b.min, p);
		b.max = glm::max(b.max, p);
	}
	return b;
}

/// <summary>
/// 全三角形の表(反時計回り)が法線と同じ向きを向いているか
/// </summary>
static bool facesMatchNormals(const MeshData& m)
{
	const int S = MeshData::STRIDE;
	for (size_t i = 0; i + 2 < m.indices.size(); i += 3)
	{
		auto p = [&](uint32_t k) { return glm::vec3(m.vertices[k * S], m.vertices[k * S + 1], m.vertices[k * S + 2]); };
		auto n = [&](uint32_t k) { return glm::vec3(m.vertices[k * S + 3], m.vertices[k * S + 4], m.vertices[k * S + 5]); };
		uint32_t a = m.indices[i], b = m.indices[i + 1], c = m.indices[i + 2];
		glm::vec3 face = glm::cross(p(b) - p(a), p(c) - p(a));
		if (glm::length(face) < 1e-8f) continue;
		if (glm::dot(face, n(a) + n(b) + n(c)) <= 0) return false;
	}
	return true;
}

int main()
{
	// 基本形: 大きさ 1、原点中心、表の向きと法線が合っている
	{
		MeshData cube = MeshFactory::cube();
		CHECK(cube.vertices.size() == 24 * MeshData::STRIDE && cube.indices.size() == 36);
		Bounds b = bounds(cube);
		CHECK(b.min == glm::vec3(-0.5f) && b.max == glm::vec3(0.5f));
		CHECK(facesMatchNormals(cube));
		CHECK(facesMatchNormals(MeshFactory::plane()));
		CHECK(bounds(MeshFactory::plane()).max.y == 0);
		CHECK(facesMatchNormals(MeshFactory::sphere()));
		CHECK(MeshFactory::builtin("builtin:sphere").has_value());
		CHECK(!MeshFactory::builtin("builtin:teapot").has_value());
	}

	// glTF: 子ノードの位置が足され、法線が作られる
	{
		auto& db = AssetDatabase::getInstance();
		Project::getInstance().open(Project::findRoot(""));
		std::string error;
		auto tree = ModelLoader::loadGltf(db.resolve("models/tree.gltf"), error);
		CHECK(tree.has_value());
		if (tree)
		{
			Bounds b = bounds(*tree);
			CHECK(std::abs(b.max.y - 2.6f) < 1e-4f); // 葉の高さ 1.8 + 子ノードの位置 0.8
			CHECK(std::abs(b.min.y) < 1e-6f);
			CHECK(std::abs(b.max.x - 0.7f) < 1e-4f);
			CHECK(facesMatchNormals(*tree));
		}
		CHECK(!ModelLoader::loadGltf(db.resolve("models/missing.gltf"), error).has_value());
		CHECK(ModelLoader::isModelFile("models/a.glb") && !ModelLoader::isModelFile("images/a.png"));
	}

	// 行列 → 位置・回転(度)・拡大 に戻すと元の値になる(ギズモで使う)
	{
		const Vec3 cases[][3] = {
			{ Vec3(1, 2, 3), Vec3(0, 0, 0), Vec3(1, 1, 1) },
			{ Vec3(-4, 0.5f, 9), Vec3(30, 45, 0), Vec3(2, 2, 2) },
			{ Vec3(0, 0, 0), Vec3(-25, 170, 10), Vec3(1, 3, 0.5f) },
			{ Vec3(5, -1, 0), Vec3(80, -60, -45), Vec3(0.2f, 1, 4) },
		};
		for (auto& c : cases)
		{
			Vec3 p, r, s;
			MathUtil::decompose(MathUtil::modelMatrix(c[0], c[1], c[2]), p, r, s);
			for (int k = 0; k < 3; k++)
			{
				CHECK(std::abs(p[k] - c[0][k]) < 1e-4f);
				CHECK(std::abs(r[k] - c[1][k]) < 1e-3f);
				CHECK(std::abs(s[k] - c[2][k]) < 1e-4f);
			}
		}
	}

	if (failures == 0) std::cout << "ModelTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
