// エディタの中身(SceneEditor)のテスト。失敗したら行番号を出して 1 を返す
#include "SceneEditor.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include <filesystem>
#include <fstream>
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
	CHECK(editor.displayName(editor.entities()[0]) == "player");
	CHECK(editor.displayName(editor.entities()[2]) == "background [1]"); // 仲間が複数のプレハブは [番号]
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

	// プレハブの値と、プレハブに戻す
	CHECK(editor.prefabField(player, "Transform", "position") == Json::parse("[0.0, 0.0, 0.0]"));
	CHECK(editor.prefabField(player, "Move", "maxSpeed") == Json(400.0));
	editor.recordUndo();
	editor.setField(player, "Move", "maxSpeed", 999);
	CHECK(editor.revertToPrefab(player, "Move", "maxSpeed"));
	player = editor.entities()[0];
	CHECK(editor.getField(player, "Move", "maxSpeed") == Json(400.0));
	editor.undo(); // 戻す前(999)に戻る
	player = editor.entities()[0];
	CHECK(editor.getField(player, "Move", "maxSpeed") == Json(999.0));
	editor.redo();
	player = editor.entities()[0];
	CHECK(!editor.revertToPrefab(player, "Name", "name")); // プレハブに無いコンポーネント

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
	CHECK(editor.entities().size() == 7 && editor.displayName(copy) == "Box #2"); // 同じ名前は番号で区別
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

	// 一時停止中は進まず、1コマ送りで1フレームだけ進む
	editor.play();
	editor.update(1.0f / 60);
	CHECK(editor.getPlayFrame() == 1);
	editor.setPaused(true);
	float x0 = editor.getField(editor.entities()[0], "Transform", "position")[0].get<float>();
	for (int i = 0; i < 5; i++) editor.update(1.0f / 60);
	CHECK(editor.getPlayFrame() == 1);
	CHECK(editor.getField(editor.entities()[0], "Transform", "position")[0].get<float>() == x0);
	editor.stepFrame();
	editor.update(1.0f / 60);
	editor.update(1.0f / 60);
	CHECK(editor.getPlayFrame() == 2);
	CHECK(editor.getField(editor.entities()[0], "Transform", "position")[0].get<float>() > x0);
	CHECK(editor.isPaused());
	editor.setPaused(false);
	editor.update(1.0f / 60);
	CHECK(editor.getPlayFrame() == 3);
	editor.stop();
	CHECK(editor.getPlayFrame() == 0 && !editor.isPaused());
	editor.setPaused(true); // 止めている時は何もしない
	CHECK(!editor.isPaused());

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

	// ---- 呼び名の番号と名前の変更
	{
		SceneEditor n(input, render, audio);
		CHECK(n.open("Field", error));
		n.duplicate(n.entities()[0]);
		auto names = n.displayNames();
		CHECK(names[0] == "player" && names.back() == "player #2");
		n.rename(n.entities().back(), "rival");
		CHECK(n.displayName(n.entities().back()) == "rival");
				n.rename(n.entities()[0], "hero");
		CHECK(n.displayNames()[0] == "hero");
		n.undo();
		CHECK(n.displayNames()[0] == "player");
		CHECK(n.isDirty());
	}

	// ---- 保存済みとの差・外で変わったファイル(assets のコピーで試す)
	{
		namespace fs = std::filesystem;
		fs::path copy = fs::temp_directory_path() / "gomi_editor_watch";
		fs::remove_all(copy);
		fs::copy(db.getRoot(), copy, fs::copy_options::recursive);
		db.setRoot(copy);
		auto writeText = [&](const std::string& id, const std::string& text) {
			std::ofstream(db.resolve(id), std::ios::binary) << text;
		};
		auto has = [](const std::vector<SceneChange>& list, SceneChange::Kind kind, const std::string& entity, const std::string& field = "") {
			for (auto& c : list)
				if (c.kind == kind && c.entity == entity && (field.empty() || c.field == field)) return true;
			return false;
		};
		using Kind = SceneChange::Kind;

		SceneEditor w(input, render, audio);
		CHECK(w.open("Field", error));
		CHECK(w.changesFromSaved().empty());
		CHECK(w.checkFiles().changed.empty()); // 最初は覚えるだけ

		// 項目の変更・Entity の追加・削除が、Entity・コンポーネント・項目の単位で出る
		Entity p = w.entities()[0];
		w.recordUndo();
		w.setField(p, "Transform", "position", Json::parse("[7, 8, 9]"));
		w.createEntity("box");
		auto changes = w.changesFromSaved();
		CHECK(changes.size() == 2);
		CHECK(has(changes, Kind::FieldChanged, "player", "position"));
		CHECK(has(changes, Kind::EntityAdded, "box"));
		CHECK(changes[0].text() == "player > Transform.position: [100.0,300.0,0.0] -> [7.0,8.0,9.0]");
		w.destroy(w.entities()[0]);
		CHECK(has(w.changesFromSaved(), Kind::EntityRemoved, "player"));
		w.undo();
		w.undo();
		w.undo();

		// 自分の保存は外の変更に数えない。保存したら差は無くなる
		p = w.entities()[0];
		w.recordUndo();
		w.setField(p, "Transform", "position", Json::parse("[1, 1, 0]"));
		CHECK(w.save(error));
		CHECK(w.changesFromSaved().empty());
		CHECK(w.checkFiles().changed.empty());

		// 外で変わった: 未保存の変更が無ければ読み直す(元に戻せる)
		Json scene = db.readJson("scenes/Field.json");
		scene["entities"][0]["overrides"]["Transform"]["position"] = Json::parse("[555, 1, 0]");
		writeText("scenes/Field.json", scene.dump(2));
		auto check = w.checkFiles();
		CHECK(check.reloaded && !check.sceneConflict && check.error.empty());
		CHECK(w.getField(w.entities()[0], "Transform", "position") == Json::parse("[555.0, 1.0, 0.0]"));
		CHECK(!w.isDirty() && w.canUndo() && w.changesFromSaved().empty());

		// 未保存の変更があれば待つ。外の変更が見え、「このまま」か「読み直す」を選べる
		w.recordUndo();
		w.setField(w.entities()[0], "Move", "maxSpeed", 1);
		scene["entities"][0]["overrides"]["Transform"]["position"] = Json::parse("[666, 1, 0]");
		writeText("scenes/Field.json", scene.dump(2));
		check = w.checkFiles();
		CHECK(check.sceneConflict && !check.reloaded && w.hasSceneConflict());
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(1.0));
		auto outside = w.fileChanges();
		CHECK(outside.size() == 1 && has(outside, Kind::FieldChanged, "player", "position"));
		w.keepEdits();
		CHECK(!w.hasSceneConflict() && w.isDirty());
		CHECK(has(w.changesFromSaved(), Kind::FieldChanged, "player", "position")); // 保存すると 666 を上書きする所

		scene["entities"][0]["overrides"]["Transform"]["position"] = Json::parse("[777, 1, 0]");
		writeText("scenes/Field.json", scene.dump(4));
		CHECK(w.checkFiles().sceneConflict);
		CHECK(w.reloadFromFile(error));
		CHECK(!w.isDirty() && !w.hasSceneConflict());
		CHECK(w.getField(w.entities()[0], "Transform", "position") == Json::parse("[777.0, 1.0, 0.0]"));
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(400.0));
		w.undo(); // 読み直す前の自分の変更に戻れる
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(1.0));
		w.redo();

		// プレハブが外で変わった: 読み直し、シーン側の変更(overrides)は残す
		Json prefab = db.readJson("prefabs/player.json");
		prefab["entities"][0]["components"]["Move"]["maxSpeed"] = 123;
		writeText("prefabs/player.json", prefab.dump(2));
		check = w.checkFiles();
		CHECK(check.refreshed && check.error.empty());
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(123.0));
		CHECK(w.getField(w.entities()[0], "Transform", "position") == Json::parse("[777.0, 1.0, 0.0]"));
		CHECK(w.changesFromSaved().empty());

		// 書きかけ・壊れたファイルは読まずに今のまま。直ったら読む
		writeText("prefabs/player.json", "{ \"entities\": [");
		check = w.checkFiles();
		CHECK(!check.error.empty() && !check.refreshed);
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(123.0));
		CHECK(w.prefabField(w.entities()[0], "Move", "maxSpeed") == Json(123.0)); // プレハブとのつながりも残る
		prefab["entities"][0]["components"]["Move"]["maxSpeed"] = 321;
		writeText("prefabs/player.json", prefab.dump(2));
		CHECK(w.checkFiles().refreshed);
		CHECK(w.getField(w.entities()[0], "Move", "maxSpeed") == Json(321.0));

		// 再生中は調べない(止めてから)
		w.play();
		scene["entities"][0]["overrides"]["Transform"]["position"] = Json::parse("[888, 1, 0]");
		writeText("scenes/Field.json", scene.dump(2));
		CHECK(w.checkFiles().changed.empty());
		w.stop();
		CHECK(w.checkFiles().sceneConflict); // やり直しで未保存になっているので待つ

		db.setRoot(AssetDatabase::findRoot(""));
		fs::remove_all(copy);
	}

	if (failures == 0) std::cout << "EditorTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
