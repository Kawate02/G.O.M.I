#pragma once
#include "Entity.h"
#include "CollisionRule.h"
#include "Vec2.h"
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
	Vec2 min;
	Vec2 max;
};
struct Circle
{
	Vec2 center;
	float radius;
};

static constexpr Cell neighbors[9] = {
	{ 1, 0 }, { 1, 1 }, { 0, 1 }, 
	{ -1, 1 }, { 0, 0 }, { -1, 0 },
	{ -1, -1 }, { 0, -1 }, { 1, -1 }
};

class CollisionContext
{
private:
	std::unordered_map<Cell, std::vector<Entity>> grid;
	Box makeBox(const TransformComponent& tf, const CollisionComponent& col, const Vec2& postmove = { 0, 0 });
	Circle makeCircle(const TransformComponent& tf, const CollisionComponent& col, const Vec2& postmove = { 0, 0 });
	bool checkAABB(const Box& box1, const Box& box2);
	bool checkCircle(const Circle& circle1, const Circle& circle2);
	bool checkCircleAABB(const Circle& circle, const Box& box);
public:
	void createGrid(World& world);
	CollisionHit queryAt(World& world, const Entity& self, EntityTag selftag, CollisionRule rule, const Vec2& postmove = { 0, 0 });
};
