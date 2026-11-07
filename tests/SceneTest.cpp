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
		CHECK(saved["systems"].size() == 10);
		CHECK(countEntities(saved) == 2); // プレハブ2つ(プレイヤー + 背景4枚)
		Env env;
		World world(env.input, env.render, env.audio);
		world.initialize(db.readJson("scenes/Field.json"), "Field");
		CHECK(world.getEntities().size() == 5);
		const Json player = SceneLoader::componentsOf(world, world.getEntities()[0]);
		CHECK(player["Transform"]["position"] == Json::parse("[100, 300, 0]"));
		CHECK(player["Transform"]["scale"] == Json::parse("[1, 1, 1]")); // プレハブ側の値が残る
		CHECK(player["Move"]["turnBrake"].dump() == "0.6");
		CHECK(SceneLoader::componentsOf(world, world.getEntities()[4])["BackGround"]["asset"] == "images/ag.png");
	}

	// プレハブのつながりは保存で残り、変えた項目だけが overrides になる
	{
		Json saved = loadAndSave(db.readJson("scenes/Field.json"), "Field");
		CHECK(saved["entities"].size() == 2);
		CHECK(saved["entities"][0] == Json::parse(R"({"prefab": "prefabs/player.json", "overrides": {"Transform": {"position": [100, 300, 0]}}})"));
		CHECK(saved["entities"][1] == Json::parse(R"({"prefab": "prefabs/background.json"})"));

		Env env;
		World world(env.input, env.render, env.audio);
		world.initialize(db.readJson("scenes/Field.json"), "Field");
		auto& cm = world.getComponentManager();
		Entity player = world.getEntities()[0];
		cm.getComponentArray<MoveComponent>().getComponent(player).maxSpeed = 123;
		cm.addComponent(player, NameComponent{ "hero" });
		Json s1 = SceneLoader::save(world);
		CHECK(s1["entities"][0]["overrides"]["Move"] == Json::parse(R"({"maxSpeed": 123})"));
		CHECK(s1["entities"][0]["overrides"]["Name"]["name"] == "hero");

		// 背景の2枚目を変えると、背景のつながりは切れて4枚とも展開される
		cm.getComponentArray<BackGroundComponent>().getComponent(world.getEntities()[2]).order = 9;
		Json s2 = SceneLoader::save(world);
		CHECK(s2["entities"].size() == 5);
		CHECK(s2["entities"][2]["components"]["BackGround"]["order"] == 9);
		CHECK(s2["entities"][0].contains("prefab"));

		// 保存したものを読み直しても同じ
		CHECK(loadAndSave(s2, "s2") == s2);

		// プレハブを後から置く
		auto made = SceneLoader::instantiate(world, "prefabs/background.json");
		CHECK(made.size() == 4);
		CHECK(world.getPrefabLink(made[3])->index == 3);
		CHECK(SceneLoader::save(world)["entities"].size() == 6);
	}

	// 破棄した Entity は保存されない
	{
		Env env;
		World world(env.input, env.render, env.audio);
		world.initialize(db.readJson("scenes/Field.json"), "Field");
		world.destroyEntity(world.getEntities()[1]);
		world.update(0.0f, false); // 破棄はフレームの最後に反映される
		Json s = SceneLoader::save(world);
		CHECK(countEntities(s) == 4); // プレイヤー + 背景の残り3枚(仲間が欠けたので展開)
		CHECK(s["entities"][0].contains("prefab"));
		CHECK(s["entities"][1].contains("components"));
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
