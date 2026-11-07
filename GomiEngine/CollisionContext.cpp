#include "CollisionContext.h"
#include "World.h"

namespace
{
	/// <summary>
	/// 1軸の重なり。どちらかの厚さが 0 の軸は境目に触れていれば重なりとみなす(2D を z = 0 で表すため)
	/// </summary>
	bool overlapAxis(float aMin, float aMax, float bMin, float bMax)
	{
		if (aMin == aMax || bMin == bMax) return aMin <= bMax && aMax >= bMin;
		return aMin < bMax && aMax > bMin;
	}
}

Box CollisionContext::makeBox(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove)
{
	Vec3 min = tf.position + postmove + col.offset;
	return { min, min + col.size };
}

Sphere CollisionContext::makeSphere(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove)
{
	return { tf.position + postmove + col.offset, col.radius };
}

Box CollisionContext::bounds(const TransformComponent& tf, const CollisionComponent& col, const Vec3& postmove)
{
	if (col.type == CollisionType::BOX) return makeBox(tf, col, postmove);
	Sphere s = makeSphere(tf, col, postmove);
	Vec3 r(s.radius, s.radius, s.radius);
	return { s.center - r, s.center + r };
}

bool CollisionContext::checkAABB(const Box& box1, const Box& box2)
{
	return overlapAxis(box1.min.x, box1.max.x, box2.min.x, box2.max.x)
		&& overlapAxis(box1.min.y, box1.max.y, box2.min.y, box2.max.y)
		&& overlapAxis(box1.min.z, box1.max.z, box2.min.z, box2.max.z);
}

bool CollisionContext::checkSphere(const Sphere& sphere1, const Sphere& sphere2)
{
	Vec3 dist = sphere1.center - sphere2.center;
	float r = sphere1.radius + sphere2.radius;
	return dist.lengthSq() <= r * r;
}

bool CollisionContext::checkSphereAABB(const Sphere& sphere, const Box& box)
{
	Vec3 closest(std::clamp(sphere.center.x, box.min.x, box.max.x),
		std::clamp(sphere.center.y, box.min.y, box.max.y),
		std::clamp(sphere.center.z, box.min.z, box.max.z));
	return (sphere.center - closest).lengthSq() <= sphere.radius * sphere.radius;
}

void CollisionContext::createGrid(World& world)
{
	grid.clear();
	auto& collisions = world.getComponentManager().getComponentArray<CollisionComponent>();
	auto& transforms = world.getComponentManager().getComponentArray<TransformComponent>();

	for (auto i = 0; i < collisions.getComponentCount(); i++)
	{
		Entity e = { collisions.getEntityId(i), collisions.getGeneration(i) };
		if (!world.alive(e)) continue;
		Box b = bounds(transforms.getComponent(e), collisions.getComponent(e));
		for (auto& c : toCell(b.min, b.max - b.min, CELL_SIZE))
		{
			grid[c].push_back(e);
		}
	}
}

CollisionHit CollisionContext::queryAt(World& world, const Entity& self, EntityTag selftag, CollisionRule rule, const Vec3& postmove)
{
	CollisionHit hit;
	auto& tag = world.getComponentManager().getComponentArray<EntityTagComponent>().getComponent(self);
	auto& collision = world.getComponentManager().getComponentArray<CollisionComponent>();
	if (!tag.has(selftag) || !collision.hasComponent(self))
	{
		return hit; //自分がタグ持ってないのは論外だよ
	}
	EntityTag othertag = rule.getPair(selftag);//相方のタグはこれしかダメだよ
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();
	auto& selftf = tf.getComponent(self);
	auto& selfcol = collision.getComponent(self);

	//移動後の位置が入るマスと、その周り(上下左右前後の 27 マス)を調べる
	std::vector<Entity> visited;
	Box selfBounds = bounds(selftf, selfcol, postmove);
	for (auto& c : toCell(selfBounds.min, selfBounds.max - selfBounds.min, CELL_SIZE))
	{
		for (int dz = -1; dz <= 1; dz++)
		for (int dy = -1; dy <= 1; dy++)
		for (int dx = -1; dx <= 1; dx++)
		{
			auto it = grid.find({ c.x + dx, c.y + dy, c.z + dz });
			if (it == grid.end()) continue;
			for (auto& otherE : it->second)
			{
				if (otherE == self) continue;//自分自身は無視
				if (std::find(visited.begin(), visited.end(), otherE) != visited.end()) continue;//既に見たエンティティは無視
				if (!world.alive(otherE)) continue;//生きてないエンティティは無視
				visited.push_back(otherE);
				auto& otherTag = world.getComponentManager().getComponentArray<EntityTagComponent>().getComponent(otherE);
				if (!otherTag.has(othertag) || !collision.hasComponent(otherE)) continue;//タグが合わない

				auto& othertf = tf.getComponent(otherE);
				auto& othercol = collision.getComponent(otherE);
				bool selfSphere = selfcol.type == CollisionType::SPHERE;
				bool otherSphere = othercol.type == CollisionType::SPHERE;
				bool touching;
				if (selfSphere && otherSphere) touching = checkSphere(makeSphere(selftf, selfcol, postmove), makeSphere(othertf, othercol));
				else if (!selfSphere && !otherSphere) touching = checkAABB(makeBox(selftf, selfcol, postmove), makeBox(othertf, othercol));
				else if (selfSphere) touching = checkSphereAABB(makeSphere(selftf, selfcol, postmove), makeBox(othertf, othercol));
				else touching = checkSphereAABB(makeSphere(othertf, othercol), makeBox(selftf, selfcol, postmove));
				if (touching) hit.entities.push_back(otherE);
			}
		}
	}
	return hit;
}
