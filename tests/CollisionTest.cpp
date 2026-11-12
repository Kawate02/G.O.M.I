// 当たり判定(3D の箱・球と、z = 0 の 2D)のテスト。失敗したら行番号を出して 1 を返す
#include "World.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

struct Env
{
	InputState input;
	RenderQueue render;
	AudioQueue audio;
	World world{ input, render, audio };

	Entity add(Vec3 pos, CollisionComponent col, EntityTag tag)
	{
		Entity e = world.createEntity();
		auto& cm = world.getComponentManager();
		cm.addComponent(e, TransformComponent{ pos, Vec3(1, 1, 1), Vec3() });
		cm.addComponent(e, col);
		cm.addComponent(e, EntityTagComponent{ tag });
		return e;
	}
	bool hits(Entity self, Vec3 move = {})
	{
		world.col().createGrid(world);
		return !world.col().queryAt(world, self, EntityTag::UNIT, collisionRules().at("unit_to_wall"), move).entities.empty();
	}
};

static CollisionComponent box(Vec3 size) { CollisionComponent c; c.type = CollisionType::BOX; c.size = size; return c; }
static CollisionComponent sphere(float r) { CollisionComponent c; c.type = CollisionType::SPHERE; c.radius = r; return c; }

int main()
{
	// 3D の箱どうし: z がずれていれば当たらない
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, box({ 1, 1, 1 }), EntityTag::UNIT);
		env.add({ 0.5f, 0.5f, 5 }, box({ 1, 1, 1 }), EntityTag::WALL);
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 0, 0, 4.6f }));   // 動いた先で重なる
		CHECK(!env.hits(me, { 0, 0, 3.9f }));  // 手前で止まる
	}
	// 球と箱、球と球
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, sphere(1), EntityTag::UNIT);
		env.add({ 0.5f, -1, -1 }, box({ 2, 2, 2 }), EntityTag::WALL);  // 箱の面まで 0.5
		CHECK(env.hits(me));
		CHECK(!env.hits(me, { -0.6f, 0, 0 }));
		Env env2;
		Entity a = env2.add({ 0, 0, 0 }, sphere(1), EntityTag::UNIT);
		env2.add({ 1.2f, 1.2f, 0.5f }, sphere(1), EntityTag::WALL);
		CHECK(env2.hits(a));                    // 距離 1.77 < 2
		CHECK(!env2.hits(a, { 0, 0, -1.5f }));  // 距離 2.4 > 2
	}
	// 2D: z の厚さ 0 どうしは平面の判定になる(元の 2D と同じ結果)
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, box({ 64, 64, 0 }), EntityTag::UNIT);
		env.add({ 100, 0, 0 }, box({ 64, 64, 0 }), EntityTag::WALL);
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 40, 0, 0 }));
		CHECK(!env.hits(me, { 36, 0, 0 }));    // x は接しただけでは当たらない(元と同じ)
		Entity c = env.add({ 32, 200, 0 }, sphere(32), EntityTag::UNIT);
		env.add({ 100, 168, 0 }, box({ 64, 64, 0 }), EntityTag::WALL);
		CHECK(env.hits(c, { 40, 0, 0 }));
	}
	// グリッドのマスをまたぐ大きな箱、遠くの物、タグ違い
	{
		Env env;
		Entity me = env.add({ 500, 1.5f, 500 }, sphere(1), EntityTag::UNIT);  // 地面の上 0.5 に浮いている
		env.add({ 0, -1, 0 }, box({ 1000, 1, 1000 }), EntityTag::WALL);  // 地面
		env.add({ 500, 1, 500 }, box({ 1, 1, 1 }), EntityTag::ENEMY);    // ルール外
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 0, -0.6f, 0 }));
		Entity far = env.add({ 5000, 5000, 5000 }, sphere(1), EntityTag::UNIT);
		CHECK(!env.hits(far, { 0, -10, 0 }));
	}

	if (failures == 0) std::cout << "CollisionTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
