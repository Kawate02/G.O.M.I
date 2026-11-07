#pragma once
#include "Entity.h"
#include "CollisionRule.h"
#include "Vec3.h"
#include "Cell.h"
#include "CellHash.h"
#include <vector>
#include <unordered_map>
#include "ComponentType.h"

#define CELL_SIZE 128
class World;

struct CollisionHit
{
	std::vector<Entity> entities;
};
struct Box
{
	Vec3 min;
	Vec3 max;
};
struct Sphere
{
	Vec3 center;
	float radius;
};

/// <summary>
/// 毎フレーム全コライダーを3Dのグリッドに登録し、周りのマスだけを調べて当たりを返す
/// </summary>
class CollisionContext
{
private:
	std::unordered_map<Cell, std::vector<Entity>> grid;
	Box makeBox(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove = {});
	Sphere makeSphere(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove = {});
	Box bounds(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove = {});
public:
	static bool checkAABB(const Box& box1, const Box& box2);
	static bool checkSphere(const Sphere& sphere1, const Sphere& sphere2);
	static bool checkSphereAABB(const Sphere& sphere, const Box& box);
	void createGrid(World& world);
	/// <summary>
	/// self を postmove だけ動かした位置で、rule の相手タグを持つ Entity と重なるものを返す
	/// </summary>
	CollisionHit queryAt(World& world, const Entity& self, EntityTag selftag, CollisionRule rule, const Vec3& postmove = {});
};
