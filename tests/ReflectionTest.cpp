#include <algorithm>
// コンポーネントの登録と JSON 変換のテスト。失敗したら行番号を出して 1 を返す
#include "ComponentRegistry.h"
#include "ComponentType.h"
#include <iostream>

// 配列・表・入れ子の構造体を持つコンポーネント(ゲームのコードで書く形)
struct TestItem
{
	std::string id;
	int count = 1;
};
GOMI_FIELDS(TestItem, id, count)

struct TestBag
{
	TestItem main;
	std::vector<TestItem> items;
	std::vector<int> numbers;
	std::vector<bool> flags;
	std::map<std::string, int> hp;
	std::unordered_map<std::string, TestItem> named;
};

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

template <typename F>
static bool throws(F f)
{
	try { f(); }
	catch (const std::exception&) { return true; }
	return false;
}

int main()
{
	registerBuiltinComponents();
	registerBuiltinComponents(); // 2回呼んでも二重登録しない
	auto& reg = ComponentRegistry::getInstance();
	CHECK(reg.all().size() == 16);

	// 全コンポーネント: 既定値 → JSON → 付け直し → JSON が一致する
	for (auto& info : reg.all())
	{
		ComponentManager cm;
		Entity e{ 0, 0 };
		info.addFromJson(cm, e, Json::object());
		CHECK(info.has(cm, e));
		Json first = info.toJson(cm, e);
		size_t omitted = 0; //既定値なら書かない項目(omitDefault)は、既定値のままの時は出ない
		for (auto& f : info.fields) { if (f.omitDefault) omitted++; }
		CHECK(first.size() == info.fields.size() - omitted);
		ComponentManager cm2;
		info.addFromJson(cm2, e, first);
		if (info.toJson(cm2, e) != first) std::cerr << "  round trip differs: " << info.name << std::endl;
		CHECK(info.toJson(cm2, e) == first);
	}

	// 値の形: Vec3 は配列、float は短い表記、ビット enum は名前の配列、map はオブジェクト
	{
		ComponentManager cm;
		Entity e{ 3, 1 };
		TagRegistry::getInstance().set({ "UNIT", "PLAYER", "WALL" }); //ふつうは project.json の "tags"
		Json j = Json::parse(R"({
			"Transform": { "position": [100, 300, 0], "rotation": [0, 0.6, 0] },
			"EntityTag": { "tags": ["UNIT", "PLAYER"] },
			"PlayerControl": { "actions": { "MOVE_RIGHT": "KEY_D", "JUMP": "KEY_SPACE" } },
			"Collision": { "type": "SPHERE", "radius": 32 }
		})");
		for (auto& [name, value] : j.items()) reg.find(name)->addFromJson(cm, e, value);

		auto& tf = cm.getComponentArray<TransformComponent>().getComponent(e);
		CHECK(tf.position == Vec3(100, 300, 0));
		CHECK(tf.scale == Vec3(0, 0, 0)); // 書いていない項目は T{} の値
		CHECK(reg.find("Transform")->toJson(cm, e)["rotation"].dump() == "[0.0,0.6,0.0]");

		auto& tag = cm.getComponentArray<EntityTagComponent>().getComponent(e);
		CHECK(tag.has("UNIT") && tag.has(TagSet::of("PLAYER")) && !tag.has("WALL"));
		CHECK(reg.find("EntityTag")->toJson(cm, e)["tags"] == Json::parse(R"(["UNIT", "PLAYER"])"));
		// project.json の tags に無い名前は、足す場所を書いたエラー
		bool unknownTag = false;
		try { reg.find("EntityTag")->addFromJson(cm, Entity{ 4, 1 }, Json::parse(R"({ "tags": ["ENEMY"] })")); }
		catch (const std::exception& ex) { unknownTag = std::string(ex.what()).find("project.json") != std::string::npos; }
		CHECK(unknownTag);
		// タグの並びの間違い
		CHECK(throws([] { TagRegistry::getInstance().loadProject(Json::parse(R"({ "tags": ["A", "A"] })")); }));
		CHECK(throws([] { TagRegistry::getInstance().loadProject(Json::parse(R"({ "tags": "A" })")); }));
		CHECK(TagRegistry::getInstance().getNames().size() == 3); // 間違えた時は今のまま

		auto& pc = cm.getComponentArray<PlayerControlComponent>().getComponent(e);
		CHECK(pc.actions.size() == 2 && pc.actions[ActionID::JUMP] == InputKey::KEY_SPACE);
		CHECK(reg.find("PlayerControl")->toJson(cm, e)["actions"]["MOVE_RIGHT"] == "KEY_D");
		// キー割り当ては「列挙 → 列挙」の表としてエディタに出す
		const FieldInfo* actions = reg.find("PlayerControl")->findField("actions");
		CHECK(actions && actions->kind == FieldKind::EnumMap);
		CHECK(actions && actions->keyNames.size() == 8 && actions->keyNames[0] == "MOVE_LEFT");
		CHECK(actions && std::find(actions->enumNames.begin(), actions->enumNames.end(), "KEY_SPACE") != actions->enumNames.end());

		CHECK(cm.getComponentArray<CollisionComponent>().getComponent(e).type == CollisionType::SPHERE);

		// コンポーネントの既定値(メンバ初期化子)が生きている
		reg.find("Physics")->addFromJson(cm, e, Json::object());
		CHECK(cm.getComponentArray<PhysicsComponent>().getComponent(e).gravity == 2400);
	}

	// 間違いは場所の分かる例外になる
	{
		ComponentManager cm;
		Entity e{ 0, 0 };
		CHECK(throws([&] { reg.find("Transform")->addFromJson(cm, e, Json::parse(R"({"positon": [0,0,0]})")); }));
		CHECK(throws([&] { reg.find("Transform")->addFromJson(cm, e, Json::parse(R"({"position": [0]})")); }));
		CHECK(throws([&] { reg.find("Collision")->addFromJson(cm, e, Json::parse(R"({"type": "TRIANGLE"})")); }));
		try { reg.find("Collision")->addFromJson(cm, e, Json::parse(R"({"type": "TRIANGLE"})")); }
		catch (const std::exception& ex) { CHECK(std::string(ex.what()).find("Collision.type") != std::string::npos); }
	}

	// エディタ向けの項目情報
	{
		auto* col = reg.find("Collision");
		CHECK(col->findField("type")->kind == FieldKind::Enum);
		CHECK(col->findField("type")->enumNames.size() == 2);
		CHECK(reg.find("EntityTag")->findField("tags")->kind == FieldKind::Tags);
		CHECK(reg.find("Transform")->findField("position")->kind == FieldKind::Vec3);
		CHECK(reg.find(std::type_index(typeid(MoveComponent)))->name == "Move");
		// 実行時の値の印
		CHECK(reg.find("Move")->findField("result")->runtime);
		CHECK(!reg.find("Move")->findField("speed")->runtime);
		CHECK(reg.find("Physics")->findField("isGround")->runtime);
	}

	// 配列・表・入れ子の構造体
	{
		reg.add<TestBag>("TestBag")
			.field("main", &TestBag::main)
			.field("items", &TestBag::items)
			.field("numbers", &TestBag::numbers)
			.field("flags", &TestBag::flags)
			.field("hp", &TestBag::hp)
			.field("named", &TestBag::named);
		auto* bag = reg.find("TestBag");
		CHECK(bag->findField("main")->kind == FieldKind::Struct);
		CHECK(bag->findField("items")->kind == FieldKind::List);
		CHECK(bag->findField("hp")->kind == FieldKind::Table);
		CHECK(bag->findField("named")->kind == FieldKind::Table);

		ComponentManager cm;
		Entity e{ 0, 0 };
		Json in = Json::parse(R"({
			"main": { "id": "sword" },
			"items": [ { "id": "potion", "count": 3 }, { "id": "key" } ],
			"numbers": [1, 2, 3],
			"flags": [true, false],
			"hp": { "alice": 10, "bob": 7 },
			"named": { "left": { "id": "shield", "count": 2 } }
		})");
		bag->addFromJson(cm, e, in);
		auto& b = cm.getComponentArray<TestBag>().getComponent(e);
		CHECK(b.main.id == "sword" && b.main.count == 1); // 書かない項目は既定値
		CHECK(b.items.size() == 2 && b.items[0].count == 3 && b.items[1].count == 1);
		CHECK(b.numbers == std::vector<int>({ 1, 2, 3 }));
		CHECK(b.flags.size() == 2 && b.flags[0] && !b.flags[1]);
		CHECK(b.hp.at("bob") == 7);
		CHECK(b.named.at("left").id == "shield");
		Json out = bag->toJson(cm, e);
		CHECK(out["main"] == Json::parse(R"({ "id": "sword", "count": 1 })"));
		CHECK(out["items"][1] == Json::parse(R"({ "id": "key", "count": 1 })"));
		CHECK(out["hp"] == in["hp"]);
		ComponentManager cm2;
		bag->addFromJson(cm2, e, out);
		CHECK(bag->toJson(cm2, e) == out);

		// 間違いは場所の分かる例外
		auto message = [&](const char* text) {
			ComponentManager c;
			try { bag->addFromJson(c, e, Json::parse(text)); }
			catch (const std::exception& ex) { return std::string(ex.what()); }
			return std::string();
		};
		CHECK(message(R"({ "items": [ { "id": "a" }, { "idd": "b" } ] })").find("TestBag.items: [1]: unknown field \"idd\"") != std::string::npos);
		CHECK(message(R"({ "hp": { "alice": "ten" } })").find("TestBag.hp: alice:") != std::string::npos);
		CHECK(message(R"({ "main": [] })").find("TestBag.main: expected an object") != std::string::npos);
		CHECK(message(R"({ "numbers": {} })").find("expected an array") != std::string::npos);
	}

	if (failures == 0) std::cout << "ReflectionTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
