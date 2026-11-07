// エディタの中身(SceneEditor)のテスト。失敗したら行番号を出して 1 を返す
#include "SceneEditor.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include <filesystem>
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

int main()
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	auto& db = AssetDatabase::getInstance();
	db.setRoot(AssetDatabase::findRoot(""));
	std::string tmp = (std::filesystem::temp_directory_path() / "gomi_editor_test.json").generic_string();

	InputState input;
	RenderQueue render;
	AudioQueue audio;
	SceneEditor editor(input, render, audio);
	std::string error;

	// 新しいシーンはカメラと光の2つ
	CHECK(editor.entities().size() == 2);
	CHECK(editor.displayName(editor.entities()[0]) == "Camera");
	CHECK(!editor.isDirty() && !editor.canUndo());

	// 開く・名前
	CHECK(editor.open("Field", error));
	CHECK(editor.getPath() == "scenes/Field.json");
	CHECK(editor.entities().size() == 5);
	CHECK(editor.displayName(editor.entities()[0]) == "player.json [0]");
	CHECK(!editor.open("NoSuchScene", error) && editor.getPath() == "scenes/Field.json");

	// 項目を変える → 元に戻す → やり直す
	Entity player = editor.entities()[0];
	editor.recordUndo();
	CHECK(editor.setField(player, "Transform", "position", Json::parse("[1, 2, 3]")));
	CHECK(editor.getField(player, "Transform", "position") == Json::parse("[1.0, 2.0, 3.0]"));
	CHECK(editor.isDirty() && editor.canUndo());
	CHECK(!editor.setField(player, "Transform", "position", Json::parse("\"bad\""), &error));
	editor.undo();
	player = editor.entities()[0];
	CHECK(editor.getField(player, "Transform", "position") == Json::parse("[100.0, 300.0, 0.0]"));
	CHECK(editor.canRedo());
	editor.redo();
	player = editor.entities()[0];
	CHECK(editor.getField(player, "Transform", "position") == Json::parse("[1.0, 2.0, 3.0]"));

	// コンポーネントの付け外し
	CHECK(editor.addComponent(player, "Name"));
	CHECK(!editor.addComponent(player, "Name")); // 二重には付かない
	editor.recordUndo();
	editor.setField(player, "Name", "name", "hero");
	CHECK(editor.displayName(player) == "hero");
	editor.removeComponent(player, "Physics");
	CHECK(editor.getField(player, "Physics", "gravity").is_null());

	// Entity の追加・複製・削除
	Entity box = editor.createEntity("Box");
	CHECK(editor.entities().size() == 6);
	editor.addComponent(box, "MeshRenderer");
	Entity copy = editor.duplicate(box);
	CHECK(editor.entities().size() == 7 && editor.displayName(copy) == "Box");
	editor.destroy(copy);
	CHECK(editor.entities().size() == 6);
	auto trees = editor.createFromPrefab("prefabs/crate.json");
	CHECK(trees.size() == 1 && editor.entities().size() == 7);
	CHECK(editor.createFromPrefab("prefabs/missing.json").empty() && editor.entities().size() == 7);

	// 保存 → 開き直すと同じ。プレイヤーは Physics を外したのでプレハブのつながりが切れて展開される
	CHECK(editor.saveAs(tmp, error));
	CHECK(!editor.isDirty());
	Json saved = db.readJson(tmp);
	CHECK(saved["entities"].size() == 4); // プレイヤー(展開) + 背景(プレハブ) + Box + 木箱(プレハブ)
	CHECK(saved["entities"][0].contains("components"));
	CHECK(saved["entities"][1] == Json::parse(R"({"prefab": "prefabs/background.json"})"));
	CHECK(saved["entities"][3] == Json::parse(R"({"prefab": "prefabs/crate.json"})"));
	CHECK(editor.open(tmp, error));
	CHECK(editor.entities().size() == 7);
	CHECK(editor.displayName(editor.entities()[0]) == "hero");

	// 再生して動いても、止めたら元に戻る(ForwardMove で右に進む)
	CHECK(editor.open("Field", error));
	player = editor.entities()[0];
	editor.update(1.0f / 60); // 止めている間は動かない
	CHECK(editor.getField(editor.entities()[0], "Transform", "position") == Json::parse("[100.0, 300.0, 0.0]"));
	editor.play();
	for (int i = 0; i < 30; i++) editor.update(1.0f / 60);
	CHECK(editor.getField(editor.entities()[0], "Transform", "position")[0].get<float>() > 100.0f);
	CHECK(!editor.canUndo());
	std::string err2;
	CHECK(!editor.save(err2)); // 再生中は保存しない
	editor.stop();
	CHECK(editor.getField(editor.entities()[0], "Transform", "position") == Json::parse("[100.0, 300.0, 0.0]"));

	// ゲーム内の「終了」は再生を止めるだけ
	editor.play();
	EventBus::getInstance().emit<int>(Event::Game_Quit, 0);
	editor.update(1.0f / 60);
	CHECK(!editor.isPlaying());

	// System の並びを変える(知らない名前は元に戻る)
	auto systems = editor.systems();
	systems.pop_back();
	editor.setSystems(systems);
	CHECK(editor.systems().size() == 9);
	editor.setSystems({ "NoSuchSystem" });
	CHECK(editor.systems().size() == 9);

	std::filesystem::remove(tmp);
	if (failures == 0) std::cout << "EditorTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
