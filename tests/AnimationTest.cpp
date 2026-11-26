// コマ送りアニメーション(クリップの読み込み・SpriteAnimationSystem・playAnimation・Renderer の uv の保存)のテスト。
// 見本のプロジェクト(samples/Demo)の assets を使う。失敗したら行番号を出して 1 を返す
#include "Project.h"
#include "SceneLoader.h"
#include "World.h"
#include "AnimationClip.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include <cmath>
#include <filesystem>
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

struct Env
{
	InputState input;
	RenderQueue render;
	AudioQueue audio;
	ResourceStore resources;
};

static bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static bool parseFails(const Json& json)
{
	try { AnimationClip::parse(json, "test"); }
	catch (const std::exception& e) { std::cerr << "  (expected) " << e.what() << std::endl; return true; }
	return false;
}

static Json animatedScene()
{
	return Json::parse(R"({
		"systems": ["SpriteAnimationSystem", "RendererSystem"],
		"entities": [ { "components": {
			"Transform": { "position": [0, 0, 0], "scale": [1, 1, 1], "rotation": [0, 0, 0] },
			"Renderer": { "path": "images/bot_walk.png" },
			"SpriteAnimation": { "clips": { "idle": "animations/bot_idle.json", "walk": "animations/bot_walk.json", "jump": "animations/bot_jump.json" }, "clip": "walk" }
		} } ],
		"ui": []
	})");
}

int main()
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	Project::getInstance().open(Project::findRoot(""));

	// シート: 4x2 の8コマ。左上から右へ、行を下へ
	{
		AnimationClip clip = AnimationClip::parse(AssetDatabase::getInstance().readJson("animations/bot_walk.json"), "bot_walk");
		CHECK(clip.frames.size() == 8);
		CHECK(clip.fps == 12 && clip.loop);
		CHECK(clip.frames[0].image == "images/bot_walk.png");
		CHECK(near(clip.frames[0].uv[0], 0) && near(clip.frames[0].uv[1], 0) && near(clip.frames[0].uv[2], 0.25f) && near(clip.frames[0].uv[3], 0.5f));
		CHECK(near(clip.frames[5].uv[0], 0.25f) && near(clip.frames[5].uv[1], 0.5f) && near(clip.frames[5].uv[2], 0.5f) && near(clip.frames[5].uv[3], 1.0f));
		CHECK(clip.frameAt(0) == 0);
		CHECK(clip.frameAt(1.0f / 12 + 0.001f) == 1);
		CHECK(clip.frameAt(8.0f / 12 + 0.001f) == 0); // ループ
		CHECK(!clip.finishedAt(100));
	}
	// count を省くと columns x rows
	{
		AnimationClip clip = AnimationClip::parse(Json::parse(R"({ "sheet": "images/bot_walk.png", "columns": 4, "rows": 2 })"), "t");
		CHECK(clip.frames.size() == 8);
	}
	// 連番: durations で2コマ分の長さ、ループしなければ最後で止まる
	{
		AnimationClip clip = AnimationClip::parse(AssetDatabase::getInstance().readJson("animations/bot_jump.json"), "bot_jump");
		CHECK(clip.frames.size() == 4 && !clip.loop);
		CHECK(clip.frames[2].image == "images/bot_jump_02.png");
		CHECK(near(clip.frames[2].uv[2], 1) && near(clip.frames[2].uv[3], 1));
		CHECK(near(clip.duration(), 0.5f));
		CHECK(clip.frameAt(0.15f) == 1);
		CHECK(clip.frameAt(0.25f) == 2);
		CHECK(clip.frameAt(0.35f) == 2); // 長さ2
		CHECK(clip.frameAt(0.45f) == 3);
		CHECK(!clip.finishedAt(0.49f));
		CHECK(clip.finishedAt(0.5f) && clip.frameAt(99) == 3);
	}
	// 厳格チェック
	CHECK(parseFails(Json::parse(R"({ "sheet": "images/bot_walk.png", "columns": 4, "rows": 2, "speed": 2 })")));       // 知らないキー
	CHECK(parseFails(Json::parse(R"({ "frames": ["images/bot_jump_00.png", "images/player.png"] })")));                   // 大きさ違い
	CHECK(parseFails(Json::parse(R"({ "frames": ["images/bot_jump_00.png", "images/nothing.png"] })")));                  // 画像が無い
	CHECK(parseFails(Json::parse(R"({ "fps": 12 })")));                                                                   // sheet も frames も無い
	CHECK(parseFails(Json::parse(R"({ "sheet": "images/bot_walk.png", "frames": ["images/bot_jump_00.png"] })")));       // 両方
	CHECK(parseFails(Json::parse(R"({ "sheet": "images/bot_walk.png", "columns": 4, "rows": 2, "count": 9 })")));        // count が多すぎる
	CHECK(parseFails(Json::parse(R"({ "frames": ["images/bot_jump_00.png"], "durations": [1, 1] })")));                  // durations の数が違う
	CHECK(parseFails(Json::parse(R"({ "frames": ["images/bot_jump_00.png"], "fps": 0 })")));                              // fps が 0
	CHECK(AnimationClip::load("animations/missing.json") == nullptr);                                                    // 落ちずに nullptr
	CHECK(AnimationClip::load("animations/bot_walk.json") != nullptr);

	// Renderer の uv・flipX は既定のままなら保存に出ない(既存のシーン・基準を変えない)。変えれば出る
	{
		Env env;
		World world(env.input, env.render, env.audio, env.resources);
		world.initialize(Json::parse(R"({ "systems": [], "entities": [ { "components": { "Transform": {}, "Renderer": { "path": "images/player.png" } } } ], "ui": [] })"), "t");
		Json saved = SceneLoader::save(world);
		CHECK(!saved["entities"][0]["components"]["Renderer"].contains("uv"));
		CHECK(!saved["entities"][0]["components"]["Renderer"].contains("flipX"));
		Json scene = Json::parse(R"({ "systems": [], "entities": [ { "components": { "Renderer": { "path": "images/player.png", "uv": [0, 0, 0.5, 1], "flipX": true } } } ], "ui": [] })");
		Env env2;
		World world2(env2.input, env2.render, env2.audio, env2.resources);
		world2.initialize(scene, "t");
		Json saved2 = SceneLoader::save(world2);
		CHECK(saved2["entities"][0]["components"]["Renderer"]["flipX"] == true);
		CHECK(saved2["entities"][0]["components"]["Renderer"]["uv"].size() == 4);
	}

	// プレハブで flipX=true、インスタンスで false に戻しても、保存して読み直すと false のまま(omitDefault が上書きを消さない)
	{
		auto& db = AssetDatabase::getInstance();
		const AssetID prefab = "prefabs/_animation_test_flipped.json";
		db.writeJson(prefab, Json::parse(R"({ "entities": [ { "components": { "Transform": {}, "Renderer": { "path": "images/player.png", "flipX": true } } } ] })"));
		Json scene = Json::parse(R"({ "systems": [], "entities": [ { "prefab": "prefabs/_animation_test_flipped.json", "overrides": { "Renderer": { "flipX": false } } } ], "ui": [] })");
		Env env;
		World world(env.input, env.render, env.audio, env.resources);
		world.initialize(scene, "t");
		Json saved = SceneLoader::save(world);
		Env env2;
		World world2(env2.input, env2.render, env2.audio, env2.resources);
		world2.initialize(saved, "saved");
		Entity e = world2.getEntities()[0];
		CHECK(!world2.getComponentManager().getComponentArray<RendererComponent>().getComponent(e).flipX);
		CHECK(SceneLoader::save(world2) == saved);
		std::filesystem::remove(db.resolve(prefab));
	}

	// 見本のシーンを読んで保存しても元と同じ(Animation を含む全シーン)
	for (auto name : { "Title", "Field", "Result", "Pickup", "Animation" })
	{
		Json first;
		{
			Env env;
			World world(env.input, env.render, env.audio, env.resources);
			AssetID path = SceneLoader::scenePath(name);
			loadCollisionSettings(Project::getInstance().read());
			try { world.initialize(AssetDatabase::getInstance().readJson(path), path); }
			catch (const std::exception&) { continue; } // ゲームのコード(DLL)が要るシーンはここでは読めない
			first = SceneLoader::save(world);
		}
		Env env;
		World world(env.input, env.render, env.audio, env.resources);
		world.initialize(first, "saved");
		CHECK(SceneLoader::save(world) == first);
	}

	// System: 今のコマを Renderer の path と uv に書き、time を進める
	{
		Env env;
		World world(env.input, env.render, env.audio, env.resources);
		world.initialize(animatedScene(), "t");
		Entity e = world.getEntities()[0];
		auto& anim = world.getComponentManager().getComponentArray<SpriteAnimationComponent>().getComponent(e);
		auto& rend = world.getComponentManager().getComponentArray<RendererComponent>().getComponent(e);
		const float dt = 1.0f / 60;

		world.update(dt);
		CHECK(rend.path == "images/bot_walk.png" && rend.uv.size() == 4 && near(rend.uv[0], 0) && near(rend.uv[2], 0.25f));
		CHECK(near(anim.time, dt));
		for (int i = 0; i < 6; i++) world.update(dt); // 7フレーム分 = 約0.117秒 → まだコマ1(0.083〜0.167)
		CHECK(near(rend.uv[0], 0.25f));
		anim.playing = false;
		float stopped = anim.time;
		world.update(dt);
		CHECK(near(anim.time, stopped)); // playing=false なら進まない
		anim.playing = true;

		// 同じ名前の play は最初に戻さない。restart=true なら戻す
		playAnimation(world, e, "walk");
		CHECK(near(anim.time, stopped));
		playAnimation(world, e, "walk", true);
		CHECK(near(anim.time, 0));
		// 知らない名前は今のクリップのまま(落ちない)
		playAnimation(world, e, "fly");
		CHECK(anim.clip == "walk");

		// ループしないクリップは最後のコマで止まり、finished が true になる
		CHECK(!isAnimationFinished(world, e));
		playAnimation(world, e, "jump");
		CHECK(anim.clip == "jump" && near(anim.time, 0));
		CHECK(!isAnimationFinished(world, e));
		for (int i = 0; i < 60; i++) world.update(dt);
		CHECK(isAnimationFinished(world, e));
		CHECK(rend.path == "images/bot_jump_03.png");
		CHECK(near(anim.time, 0.5f)); // 最後で止める(増え続けない)
		playAnimation(world, e, "walk");
		CHECK(!isAnimationFinished(world, e)); // ループするクリップは常に false
	}

	// 状態は time・clip・playing だけ: 保存して読み直し、同じだけ進めると同じコマになる(巻き戻しでずれない)
	{
		Env env;
		World world(env.input, env.render, env.audio, env.resources);
		world.initialize(animatedScene(), "t");
		for (int i = 0; i < 20; i++) world.update(1.0f / 60);
		Json saved = SceneLoader::save(world);
		world.update(1.0f / 60);
		Json expected = SceneLoader::save(world);

		Env env2;
		World world2(env2.input, env2.render, env2.audio, env2.resources);
		world2.initialize(saved, "saved");
		world2.update(1.0f / 60);
		CHECK(SceneLoader::save(world2) == expected);
	}

	if (failures == 0) std::cout << "AnimationTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
