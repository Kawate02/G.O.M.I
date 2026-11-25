// 当たり判定(3D の箱・球と、z = 0 の 2D)・ルールの読み込み・BLOCK・TRIGGER のイベントのテスト。失敗したら行番号を出して 1 を返す
#include "World.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "CollisionSystem.h"
#include "SceneLoader.h"
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

static TagSet tag(const char* name) { return TagSet::of(name); }

struct Env
{
	InputState input;
	RenderQueue render;
	AudioQueue audio;
	ResourceStore resources;
	World world{ input, render, audio, resources };

	Entity add(Vec3 pos, CollisionComponent col, TagSet tags)
	{
		Entity e = world.createEntity();
		auto& cm = world.getComponentManager();
		cm.addComponent(e, TransformComponent{ pos, Vec3(1, 1, 1), Vec3() });
		cm.addComponent(e, col);
		cm.addComponent(e, EntityTagComponent{ tags });
		return e;
	}
	bool hits(Entity self, Vec3 move = {})
	{
		world.col().createGrid(world);
		return !world.col().queryAt(world, self, tag("UNIT"), tag("WALL"), move).entities.empty();
	}
	bool blocked(Entity self, Vec3 move = {})
	{
		world.col().createGrid(world);
		return world.col().blocked(world, self, move);
	}
};

static CollisionComponent box(Vec3 size) { CollisionComponent c; c.type = CollisionType::BOX; c.size = size; return c; }
static CollisionComponent sphere(float r) { CollisionComponent c; c.type = CollisionType::SPHERE; c.radius = r; return c; }

template <typename F>
static std::string errorOf(F f)
{
	try { f(); }
	catch (const std::exception& e) { return e.what(); }
	return "";
}

int main()
{
	// タグとルールはふつう project.json と assets/collision_rules.json から読む
	TagRegistry::getInstance().set({ "UNIT", "WALL", "ENEMY", "PLAYER", "ITEM" });
	CollisionRules::getInstance().set(CollisionRules::parse(Json::parse(R"({
		"unit_to_wall": { "a": "UNIT", "b": "WALL", "kind": "BLOCK" },
		"player_item": { "a": "PLAYER", "b": "ITEM", "kind": "TRIGGER" }
	})")));

	// 3D の箱どうし: z がずれていれば当たらない
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, box({ 1, 1, 1 }), tag("UNIT"));
		env.add({ 0.5f, 0.5f, 5 }, box({ 1, 1, 1 }), tag("WALL"));
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 0, 0, 4.6f }));   // 動いた先で重なる
		CHECK(!env.hits(me, { 0, 0, 3.9f }));  // 手前で止まる
	}
	// 球と箱、球と球
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, sphere(1), tag("UNIT"));
		env.add({ 0.5f, -1, -1 }, box({ 2, 2, 2 }), tag("WALL"));  // 箱の面まで 0.5
		CHECK(env.hits(me));
		CHECK(!env.hits(me, { -0.6f, 0, 0 }));
		Env env2;
		Entity a = env2.add({ 0, 0, 0 }, sphere(1), tag("UNIT"));
		env2.add({ 1.2f, 1.2f, 0.5f }, sphere(1), tag("WALL"));
		CHECK(env2.hits(a));                    // 距離 1.77 < 2
		CHECK(!env2.hits(a, { 0, 0, -1.5f }));  // 距離 2.4 > 2
	}
	// 2D: z の厚さ 0 どうしは平面の判定になる(元の 2D と同じ結果)
	{
		Env env;
		Entity me = env.add({ 0, 0, 0 }, box({ 64, 64, 0 }), tag("UNIT"));
		env.add({ 100, 0, 0 }, box({ 64, 64, 0 }), tag("WALL"));
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 40, 0, 0 }));
		CHECK(!env.hits(me, { 36, 0, 0 }));    // x は接しただけでは当たらない(元と同じ)
		Entity c = env.add({ 32, 200, 0 }, sphere(32), tag("UNIT"));
		env.add({ 100, 168, 0 }, box({ 64, 64, 0 }), tag("WALL"));
		CHECK(env.hits(c, { 40, 0, 0 }));
	}
	// グリッドのマスをまたぐ大きな箱、遠くの物、タグ違い
	{
		Env env;
		Entity me = env.add({ 500, 1.5f, 500 }, sphere(1), tag("UNIT"));  // 地面の上 0.5 に浮いている
		env.add({ 0, -1, 0 }, box({ 1000, 1, 1000 }), tag("WALL"));  // 地面
		env.add({ 500, 1, 500 }, box({ 1, 1, 1 }), tag("ENEMY"));    // ルール外
		CHECK(!env.hits(me));
		CHECK(env.hits(me, { 0, -0.6f, 0 }));
		Entity far = env.add({ 5000, 5000, 5000 }, sphere(1), tag("UNIT"));
		CHECK(!env.hits(far, { 0, -10, 0 }));
	}
	// BLOCK: ルールの a 側も b 側も止まる。TRIGGER のルールでは止まらない
	{
		Env env;
		Entity unit = env.add({ 0, 0, 0 }, box({ 1, 1, 1 }), tag("UNIT"));
		Entity wall = env.add({ 2, 0, 0 }, box({ 1, 1, 1 }), tag("WALL"));
		Entity player = env.add({ 0, 5, 0 }, box({ 1, 1, 1 }), tag("PLAYER"));
		env.add({ 2, 5, 0 }, box({ 1, 1, 1 }), tag("ITEM"));
		CHECK(env.blocked(unit, { 1.5f, 0, 0 }));
		CHECK(env.blocked(wall, { -1.5f, 0, 0 }));
		CHECK(!env.blocked(player, { 1.5f, 0, 0 }));
		CHECK(!env.blocked(unit, { 0.5f, 0, 0 }));
	}
	// ルールの JSON の間違いはファイル名とルールID付き
	{
		auto bad = [](const char* json) { return errorOf([&] { CollisionRules::parse(Json::parse(json)); }); };
		CHECK(bad(R"({ "r": { "a": "UNIT", "b": "WAL", "kind": "BLOCK" } })").find("collision_rules.json: r: b: unknown tag \"WAL\"") == 0);
		CHECK(bad(R"({ "r": { "a": "UNIT", "b": "WALL", "kind": "DAMAGE" } })").find("unknown kind") != std::string::npos);
		CHECK(bad(R"({ "r": { "a": "UNIT", "b": "WALL" } })").find("\"kind\" is missing") != std::string::npos);
		CHECK(bad(R"({ "r": { "a": "UNIT", "b": "WALL", "kind": "BLOCK", "x": 1 } })").find("unknown field \"x\"") != std::string::npos);
		CHECK(bad(R"([])").find("collision_rules.json") == 0);
		CHECK(bad(R"({ "r": { "a": "UNIT", "b": "WALL", "kind": "TRIGGER" } })").empty());
	}
	// TRIGGER: 入った・重なり中・出たを両方に出す。相手が消えたら残った側に EXIT。順は毎回同じ
	{
		Env env;
		CollisionSystem sys;
		sys.initialize(env.world);
		Entity player = env.add({ 0, 0, 0 }, box({ 1, 1, 0 }), tag("PLAYER"));
		Entity item1 = env.add({ 3, 0, 0 }, box({ 1, 1, 0 }), tag("ITEM"));
		Entity item2 = env.add({ 0, 0.5f, 0 }, box({ 1, 1, 0 }), tag("ITEM"));
		auto& tf = env.world.getComponentManager().getComponentArray<TransformComponent>();
		auto step = [&] {
			env.world.event().clearEvents();
			env.world.col().createGrid(env.world);
			sys.update(env.world, 0, 0);
			return env.world.event().collisions();
		};
		auto is = [](const CollisionEvent& ev, Entity self, Entity other, CollisionPhase phase) {
			return ev.self == self && ev.other == other && ev.rule == "player_item" && ev.phase == phase;
		};

		auto f1 = step(); // item2 とだけ重なっている
		CHECK(f1.size() == 2);
		CHECK(f1.size() == 2 && is(f1[0], player, item2, CollisionPhase::ENTER) && is(f1[1], item2, player, CollisionPhase::ENTER));
		CHECK(env.world.event().collisionsOf(item1).empty());

		auto f2 = step(); // そのまま
		CHECK(f2.size() == 2 && is(f2[0], player, item2, CollisionPhase::STAY) && is(f2[1], item2, player, CollisionPhase::STAY));

		tf.getComponent(player).position = Vec3(2.5f, 0, 0); // item2 から離れ item1 に入る
		auto f3 = step();
		CHECK(f3.size() == 4);
		CHECK(f3.size() == 4 && is(f3[0], player, item1, CollisionPhase::ENTER) && is(f3[1], item1, player, CollisionPhase::ENTER)
			&& is(f3[2], player, item2, CollisionPhase::EXIT) && is(f3[3], item2, player, CollisionPhase::EXIT));

		env.world.destroyEntity(item1); // 消えた → 残った player にだけ EXIT
		auto f4 = step();
		CHECK(f4.size() == 1 && is(f4[0], player, item1, CollisionPhase::EXIT));

		auto f5 = step();
		CHECK(f5.empty());
	}

	// 巻き戻し: 重なりの組を runState に入れて戻すと、そのまま進めた時と同じ STAY が出る(Entity の ID が変わっても)
	{
		registerBuiltinComponents(); // シーンの保存と読み込みに要る
		Env a;
		CollisionSystem sysA;
		sysA.initialize(a.world);
		Entity dead = a.add({ 50, 50, 0 }, box({ 1, 1, 0 }), tag("ENEMY")); // 消して ID をずらす
		Entity player = a.add({ 0, 0, 0 }, box({ 1, 1, 0 }), tag("PLAYER"));
		Entity item = a.add({ 0, 0.5f, 0 }, box({ 1, 1, 0 }), tag("ITEM"));
		a.world.destroyEntity(dead);
		a.world.update(0, false); // 消えた ID の掃除
		auto stepA = [&] {
			a.world.event().clearEvents();
			a.world.col().createGrid(a.world);
			sysA.update(a.world, 0, 0);
			return a.world.event().collisions();
		};
		stepA(); // ENTER
		std::vector<Entity> order;
		Json saved = SceneLoader::save(a.world, &order);
		Json state = sysA.runState(order);
		auto expect = stepA(); // そのまま進めた時(STAY)

		Env b;
		b.world.initialize(saved, "test");
		CollisionSystem sysB;
		sysB.initialize(b.world);
		sysB.restoreRunState(state, b.world.getEntities());
		b.world.event().clearEvents();
		b.world.col().createGrid(b.world);
		sysB.update(b.world, 0, 0);
		auto got = b.world.event().collisions();
		CHECK(expect.size() == 2 && expect[0].phase == CollisionPhase::STAY);
		CHECK(got.size() == expect.size());
		for (size_t i = 0; i < got.size() && i < expect.size(); i++) CHECK(got[i].phase == expect[i].phase && got[i].rule == expect[i].rule);
		CHECK(player.id != b.world.getEntities()[0].id); // ID は確かにずれている
	}

	if (failures == 0) std::cout << "CollisionTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
