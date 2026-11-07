// シーンの読み込み・保存のテスト(assets フォルダの実ファイルを使う)。失敗したら行番号を出して 1 を返す
#include "SceneLoader.h"
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
};

static Json loadAndSave(const Json& scene, const std::string& source)
{
	Env env;
	World world(env.input, env.render, env.audio);
	world.initialize(scene, source);
	return SceneLoader::save(world);
}

static int countEntities(const Json& saved)
{
	return static_cast<int>(saved["entities"].size());
}

int main()
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	auto& db = AssetDatabase::getInstance();
	db.setRoot(AssetDatabase::findRoot(""));

	// 全シーン: 読む → 保存 → 保存したものを読む → 保存、が一致する
	for (auto name : { "Title", "Field", "Result" })
	{
		AssetID path = SceneLoader::scenePath(name);
		Json first = loadAndSave(db.readJson(path), path);
		Json second = loadAndSave(first, "saved");
		if (first != second) std::cerr << "  round trip differs: " << name << std::endl;
		CHECK(first == second);
	}

	// Field の中身: プレイヤー1体 + 背景4枚、overrides が効いている
	{
		Json saved = loadAndSave(db.readJson("scenes/Field.json"), "Field");
		CHECK(countEntities(saved) == 5);
		CHECK(saved["systems"].size() == 10);
		const Json& player = saved["entities"][0]["components"];
		CHECK(player["Transform"]["position"] == Json::parse("[100, 300, 0]"));
		CHECK(player["Transform"]["scale"] == Json::parse("[1, 1, 1]")); // プレハブ側の値が残る
		CHECK(player["Move"]["turnBrake"].dump() == "0.6");
		CHECK(saved["entities"][4]["components"]["BackGround"]["asset"] == "images/ag.png");
	}

	// 破棄した Entity は保存されない
	{
		Env env;
		World world(env.input, env.render, env.audio);
		world.initialize(db.readJson("scenes/Field.json"), "Field");
		world.destroyEntity(world.getEntities()[1]);
		CHECK(countEntities(SceneLoader::save(world)) == 4);
	}

	// merge: オブジェクトは中まで重ね、それ以外は置き換え
	{
		Json base = Json::parse(R"({"Transform": {"position": [0,0,0], "angle": 1}, "Move": {"speed": 5}})");
		Json patch = Json::parse(R"({"Transform": {"position": [1,2,3]}, "Tag": {"x": 1}})");
		Json merged = SceneLoader::merge(base, patch);
		CHECK(merged["Transform"]["position"] == Json::parse("[1,2,3]"));
		CHECK(merged["Transform"]["angle"] == 1);
		CHECK(merged["Move"]["speed"] == 5);
		CHECK(merged["Tag"]["x"] == 1);
	}

	// 間違ったシーンは読めない
	{
		Env env;
		World world(env.input, env.render, env.audio);
		bool threw = false;
		try { world.initialize(Json::parse(R"({"systems": ["NoSuchSystem"]})"), "bad"); }
		catch (const std::exception& e) { threw = std::string(e.what()).find("NoSuchSystem") != std::string::npos; }
		CHECK(threw);
	}

	CHECK(SceneLoader::scenePath("Field") == "scenes/Field.json");
	CHECK(SceneLoader::scenePath("C:/tmp/a.json") == "C:/tmp/a.json");

	if (failures == 0) std::cout << "SceneTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
