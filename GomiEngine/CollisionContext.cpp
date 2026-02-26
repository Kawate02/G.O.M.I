#include "CollisionContext.h"
#include "World.h"

Box CollisionContext::makeBox(const TransformComponent& tf, const CollisionComponent& col, const Vec2& postmove)
{
	Vec2 min = { tf.position.x + postmove.x + col.offset.x, tf.position.y + postmove.y + col.offset.y };
	return { min, min + col.size };
}

Circle CollisionContext::makeCircle(const TransformComponent& tf, const CollisionComponent& col, const Vec2& postmove)
{
	return { { tf.position.x + postmove.x + col.offset.x, tf.position.y + postmove.y + col.offset.y }, col.radius };
}

bool CollisionContext::checkAABB(const Box& box1, const Box& box2)
{
	if (box1.min.x < box2.max.x &&
		box1.max.x > box2.min.x &&
		box1.min.y < box2.max.y &&
		box1.max.y > box2.min.y)
	{
		return true;
	}
	return false;
}

bool CollisionContext::checkCircle(const Circle& circle1, const Circle& circle2)
{
	Vec2 dist = circle1.center - circle2.center;
	float r = circle1.radius + circle2.radius;
	return dist.lengthSq() <= r * r;
}

bool CollisionContext::checkCircleAABB(const Circle& circle, const Box& box)
{
	float closestX = std::clamp(circle.center.x, box.min.x, box.max.x);
	float closestY = std::clamp(circle.center.y, box.min.y, box.max.y);

	float dx = circle.center.x - closestX;
	float dy = circle.center.y - closestY;

	return dx * dx + dy * dy <= circle.radius * circle.radius;
}

void CollisionContext::createGrid(World& world)
{
	grid.clear();
	auto& entities = world.getEntities();
	auto& collisions = world.getComponentManager().getComponentArray<CollisionComponent>();

	for (auto i = 0; i < collisions.getComponentCount(); i++)
	{
		auto& e = entities[collisions.getEntityId(i)];
		auto& colc = collisions.getComponent(e);
		auto& tf = world.getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
		std::vector<Cell> cell;
		if (colc.type == CollisionType::BOX) cell = toCell({ tf.position.x + colc.offset.x, tf.position.y + colc.offset.y }, colc.size, CELL_SIZE);
		if (colc.type == CollisionType::CIRCLE) cell = toCell({ tf.position.x + colc.offset.x - colc.radius, tf.position.y + colc.offset.y - colc.radius}, { colc.radius * 2, colc.radius * 2 }, CELL_SIZE);

		for (auto& c : cell)
		{
			grid[c].push_back(e);
		}
	}
}

CollisionHit CollisionContext::queryAt(World& world, const Entity& self, EntityTag selftag, CollisionRule rule, const Vec2& postmove)
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
	std::vector<Entity> visited;
	std::vector<Cell> cells;
	auto& selftf = tf.getComponent(self);
	auto& selfcol = collision.getComponent(self);
	if (postmove.x != 0 || postmove.y != 0)
	{
		if (selfcol.type == CollisionType::BOX)
		{
			cells = toCell({
				selftf.position.x + selfcol.offset.x + postmove.x,
				selftf.position.y + selfcol.offset.y + postmove.y }, {
				selfcol.size }, CELL_SIZE);
		}
		if (selfcol.type == CollisionType::CIRCLE)
		{
			cells = toCell({ 
				selftf.position.x + selfcol.offset.x - selfcol.radius + postmove.x,
				selftf.position.y + selfcol.offset.y - selfcol.radius + postmove.y }, {
				selfcol.radius * 2 , selfcol.radius * 2 }, CELL_SIZE);
		}
	}

	for (auto& c : cells)
	{
		for (int i = 0; i < sizeof(neighbors) / sizeof(neighbors[0]); i++)//周囲含めた9マスを調べるよ
		{
			Cell cell = { c.x + neighbors[i].x, c.y + neighbors[i].y };
			auto it = grid.find(cell);
			if (it != grid.end())//周りにエンティティがいた
			{
				for (auto& otherE : it->second)
				{
					if (otherE == self) continue;//自分自身は無視
					if (std::find(visited.begin(), visited.end(), otherE) != visited.end()) continue;//既に見たエンティティは無視
					if (!world.alive(otherE)) continue;//生きてないエンティティは無視
					visited.push_back(otherE);
					auto& otherTag = world.getComponentManager().getComponentArray<EntityTagComponent>().getComponent(otherE);
					if (otherTag.has(othertag) && collision.hasComponent(otherE))//タグも合ってる
					{
						auto& othertf = tf.getComponent(otherE);
						auto& othercol = collision.getComponent(otherE);

						if (selfcol.type == CollisionType::CIRCLE && othercol.type == CollisionType::CIRCLE)
						{
							if (checkCircle(makeCircle(selftf, selfcol, postmove), makeCircle(othertf, othercol)))
							{
								hit.entities.push_back(otherE);
							}
						}
						if (selfcol.type == CollisionType::BOX && othercol.type == CollisionType::BOX)
						{
							if (checkAABB(makeBox(selftf, selfcol, postmove), makeBox(othertf, othercol)))
							{
								hit.entities.push_back(otherE);
							}
						}
						if (selfcol.type == CollisionType::CIRCLE && othercol.type == CollisionType::BOX)
						{
							if (checkCircleAABB(makeCircle(selftf, selfcol, postmove), makeBox(othertf, othercol)))
							{
								hit.entities.push_back(otherE);
							}
						}
						if (selfcol.type == CollisionType::BOX && othercol.type == CollisionType::CIRCLE)
						{
							if (checkCircleAABB(makeCircle(othertf, othercol), makeBox(selftf, selfcol, postmove)))
							{
								hit.entities.push_back(otherE);
							}
						}
					}
				}
			}
		}
	}
	return hit;
}
