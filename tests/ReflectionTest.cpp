// コンポーネントの登録と JSON 変換のテスト。失敗したら行番号を出して 1 を返す
#include "ComponentRegistry.h"
#include "ComponentType.h"
#include <iostream>

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
	CHECK(reg.all().size() == 14);

	// 全コンポーネント: 既定値 → JSON → 付け直し → JSON が一致する
	for (auto& info : reg.all())
	{
		ComponentManager cm;
		Entity e{ 0, 0 };
		info.addFromJson(cm, e, Json::object());
		CHECK(info.has(cm, e));
		Json first = info.toJson(cm, e);
		CHECK(first.size() == info.fields.size());
		ComponentManager cm2;
		info.addFromJson(cm2, e, first);
		if (info.toJson(cm2, e) != first) std::cerr << "  round trip differs: " << info.name << std::endl;
		CHECK(info.toJson(cm2, e) == first);
	}

	// 値の形: Vec3 は配列、float は短い表記、ビット enum は名前の配列、map はオブジェクト
	{
		ComponentManager cm;
		Entity e{ 3, 1 };
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
		CHECK(tag.has(EntityTag::UNIT) && tag.has(EntityTag::PLAYER) && !tag.has(EntityTag::WALL));
		CHECK(reg.find("EntityTag")->toJson(cm, e)["tags"] == Json::parse(R"(["UNIT", "PLAYER"])"));

		auto& pc = cm.getComponentArray<PlayerControlComponent>().getComponent(e);
		CHECK(pc.actions.size() == 2 && pc.actions[ActionID::JUMP] == InputKey::KEY_SPACE);
		CHECK(reg.find("PlayerControl")->toJson(cm, e)["actions"]["MOVE_RIGHT"] == "KEY_D");

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
		CHECK(reg.find("EntityTag")->findField("tags")->kind == FieldKind::Flags);
		CHECK(reg.find("Transform")->findField("position")->kind == FieldKind::Vec3);
		CHECK(reg.find(std::type_index(typeid(MoveComponent)))->name == "Move");
	}

	if (failures == 0) std::cout << "ReflectionTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
