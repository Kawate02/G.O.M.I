// 操作の記録・再生・比べる処理のテスト。失敗したら行番号を出して 1 を返す
#include "Replay.h"
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

int main()
{
	namespace fs = std::filesystem;

	// 記録: 変わった所だけが残る
	ReplayRecorder rec("Field", 60, 7);
	InputState s;
	s.mouse.position = Vec2(10, 20);
	rec.record(1, s); // 最初のフレームはマウスの位置を必ず残す
	rec.record(2, s); // 何も変わらない
	s.key[static_cast<int>(InputKey::KEY_D)] = 1;
	rec.record(3, s);
	s.mouse.position = Vec2(15, 20);
	rec.record(4, s);
	s.key[static_cast<int>(InputKey::KEY_D)] = 0;
	s.mouse.deltaWheel = -1;
	rec.record(5, s);
	s.mouse.deltaWheel = 0;
	Replay r = rec.finish(250, 120);
	CHECK(r.events.size() == 4);
	CHECK(r.checkpoints == std::vector<int>({ 120, 240, 250 }));
	CHECK(r.scene == "Field" && r.fps == 60 && r.seed == 7);

	// JSON の行き来
	fs::path tmp = fs::temp_directory_path() / "gomi_replay_test" / "case.json";
	r.save(tmp);
	Replay loaded = Replay::load(tmp);
	CHECK(loaded.toJson() == r.toJson());
	CHECK(Replay::expectedDir(tmp) == tmp.parent_path() / "case");

	// 再生: 押した瞬間・離した瞬間・マウスの移動量が作り直される
	ReplayPlayer player(loaded);
	InputState out;
	out.key[static_cast<int>(InputKey::KEY_A)] = 1; // 実際の入力は捨てられる
	player.apply(1, out);
	CHECK(out.mouse.position.x == 10 && out.mouse.deltaPosition.x == 0);
	CHECK(!out.isPressedKey(InputKey::KEY_A));
	player.apply(2, out);
	player.apply(3, out);
	CHECK(out.isDownKey(InputKey::KEY_D));
	player.apply(4, out);
	CHECK(out.isPressedKey(InputKey::KEY_D) && !out.isDownKey(InputKey::KEY_D));
	CHECK(out.mouse.deltaPosition.x == 5);
	player.apply(5, out);
	CHECK(out.isUpKey(InputKey::KEY_D) && out.mouse.deltaWheel == -1 && out.mouse.totalWheel == -1);
	player.apply(6, out);
	CHECK(!out.isUpKey(InputKey::KEY_D) && out.mouse.deltaWheel == 0 && out.mouse.deltaPosition.x == 0);

	// 厳しく読む
	auto throws = [](const char* text) {
		try { Replay::fromJson(Json::parse(text)); }
		catch (const std::exception&) { return true; }
		return false;
	};
	CHECK(throws(R"({"length": 10, "typo": 1})"));
	CHECK(throws(R"({"length": 10, "input": [{"frame": 1, "press": ["NO_SUCH_KEY"]}]})"));
	CHECK(throws(R"({"length": 10, "checkpoints": [11]})"));
	CHECK(throws(R"({"length": 10, "input": [{"frame": 5}, {"frame": 3}]})"));
	CHECK(!throws(R"({"length": 10, "input": [{"frame": 1, "press": ["KEY_SPACE"]}]})"));

	// 状態の差分
	Json a = Json::parse(R"({"x": 1, "list": [1, 2], "o": {"k": "a"}})");
	Json b = Json::parse(R"({"x": 2, "list": [1, 2, 3], "o": {"k": "a"}, "new": true})");
	auto diffs = ReplayCheck::jsonDiff(a, b);
	CHECK(diffs.size() == 3);
	CHECK(std::find(diffs.begin(), diffs.end(), "x: 1 -> 2") != diffs.end());
	CHECK(ReplayCheck::jsonDiff(a, a).empty());
	CHECK(ReplayCheck::jsonDiff(a, b, 1).size() == 2); // 1件 + 「ほか」

	// 基準を作ってから比べる(画面は小さな画像を書く関数で代わりにする)
	fs::path dir = tmp.parent_path();
	fs::remove_all(dir / "expected");
	unsigned char color = 0;
	auto capture = [&](const std::string& path) {
		std::vector<unsigned char> px(4 * 4 * 4, color);
		return stbi_write_png(path.c_str(), 4, 4, 4, px.data(), 16) != 0;
	};
	Replay one;
	one.length = 1;
	one.checkpoints = { 1 };
	ReplayCheck update(dir / "expected", dir / "out", ReplayCheck::Mode::Update);
	update.checkpoint(1, a, capture);
	CHECK(update.passed(one));
	ReplayCheck same(dir / "expected", dir / "out", ReplayCheck::Mode::Check);
	CHECK(same.checkpoint(1, a, capture).ok());
	CHECK(same.passed(one));
	color = 255;
	ReplayCheck differ(dir / "expected", dir / "out", ReplayCheck::Mode::Check);
	auto& res = differ.checkpoint(1, b, capture);
	CHECK(!res.stateOk() && res.pixelDiffs == 16 && fs::exists(res.diffImage));
	CHECK(!differ.passed(one));
	ReplayCheck notReached(dir / "expected", dir / "out", ReplayCheck::Mode::Check);
	CHECK(!notReached.passed(one));

	if (failures) std::cerr << failures << " failure(s)" << std::endl;
	else std::cout << "ReplayTest passed" << std::endl;
	return failures ? 1 : 0;
}
