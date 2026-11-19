// ゲームのコードの DLL(見本は tests/game_sample)を読み、登録・動作・外した後を確かめる。失敗したら行番号を出して 1 を返す
#include "GameModule.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "ResourceRegistry.h"
#include "SceneLoader.h"
#include "World.h"
#include <iostream>
#include <memory>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

struct Env
{
	InputState input;
	RenderQueue render;
	AudioQueue audio;
	ResourceStore resources;
};

static int hpOf(World& world)
{
	return SceneLoader::componentsOf(world, world.getEntities()[0])["SampleHp"]["hp"].get<int>();
}

int main()
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	auto& components = ComponentRegistry::getInstance();
	auto& systems = SystemRegistry::getInstance();
	const size_t builtinComponents = components.size();
	const size_t builtinSystems = systems.size();
	const auto dll = GameModule::exeDir() / "GameSampleModule.dll";
	const Json scene = Json::parse(R"({
		"systems": ["SampleDamageSystem"],
		"entities": [ { "components": { "SampleHp": { "hp": 5 } } } ]
	})");

	for (int round = 0; round < 2; round++) // 外して読み直しても同じに動く
	{
		GameModule module;
		std::string error;
		CHECK(module.load(dll, error));
		if (!error.empty()) std::cerr << "  " << error << std::endl;
		CHECK(components.find("SampleHp") != nullptr);
		CHECK(systems.contains("SampleDamageSystem"));
		CHECK(ResourceRegistry::getInstance().find("SampleScore") != nullptr);
		{
			Env env;
			auto world = std::make_unique<World>(env.input, env.render, env.audio, env.resources);
			world->initialize(scene, "test");
			CHECK(hpOf(*world) == 5);
			world->update(1.0f / 60);
			world->update(1.0f / 60);
			CHECK(hpOf(*world) == 3);
			// Resource は World を作り直しても残る
			CHECK(env.resources.save()["SampleScore"]["hits"] == 2);
			world->release();
			world = std::make_unique<World>(env.input, env.render, env.audio, env.resources);
			world->initialize(scene, "test");
			world->update(1.0f / 60);
			CHECK(env.resources.save()["SampleScore"]["hits"] == 3);
			// 初期値の上書き: 書いた項目だけ、知らない名前・項目は名前付きの例外で値は変わらない
			env.resources.reset(Json::parse(R"({ "SampleScore": { "log": ["a"] } })"));
			CHECK(env.resources.save()["SampleScore"] == Json::parse(R"({ "hits": 0, "log": ["a"] })"));
			std::string message;
			try { env.resources.reset(Json::parse(R"({ "SampleScore": { "hit": 1 } })"), "resources.json"); }
			catch (const std::exception& ex) { message = ex.what(); }
			CHECK(message.find("resources.json: SampleScore: unknown field \"hit\"") != std::string::npos);
			message.clear();
			try { env.resources.reset(Json::parse(R"({ "Score": {} })"), "resources.json"); }
			catch (const std::exception& ex) { message = ex.what(); }
			CHECK(message.find("resources.json: unknown resource \"Score\"") != std::string::npos);
			CHECK(env.resources.save()["SampleScore"]["log"] == Json::parse(R"(["a"])"));
			env.resources.clear();
		} // World(DLL の型の配列と System)と Resource の値を先に消す
		module.unload();
		CHECK(ResourceRegistry::getInstance().size() == 0);
		CHECK(components.size() == builtinComponents);
		CHECK(systems.size() == builtinSystems);
		CHECK(components.find("SampleHp") == nullptr);
	}

	// 組み込みと同じ名前は登録できない
	CHECK(components.find("Transform") != nullptr);
	bool threw = false;
	struct Dummy { int v = 0; };
	try { components.add<Dummy>("Transform"); }
	catch (const std::exception&) { threw = true; }
	CHECK(threw);
	CHECK(components.size() == builtinComponents);

	// 無い DLL は理由付きで失敗する
	GameModule missing;
	std::string error;
	CHECK(!missing.load(GameModule::exeDir() / "NoSuchGame.dll", error));
	CHECK(!error.empty());

	if (failures == 0) std::cout << "GameModuleTest: OK" << std::endl;
	return failures == 0 ? 0 : 1;
}
