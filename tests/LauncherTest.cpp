// ランチャーの中身(LauncherCore)のテスト。gh は使わず、作った zip から入れる。失敗したら行番号を出して 1 を返す
#include "LauncherCore.h"
#include <algorithm>
#include <fstream>
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

using namespace launcher;

static void writeFile(const fs::path& path, const std::string& text)
{
	fs::create_directories(path.parent_path());
	std::ofstream(path, std::ios::binary) << text;
}

static std::string readFile(const fs::path& path)
{
	std::ifstream in(path, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(in), {});
}

// 配布物と同じ形の zip(中は GOMI-<番号>/ 1つ)を作る
static fs::path makeZip(const fs::path& work, const std::string& top)
{
	fs::path src = work / ("src-" + top);
	writeFile(src / top / "GomiEditor.exe", "");
	writeFile(src / top / "templates" / "Demo" / "project.json", "{\n  \"engine\": \"0.0.1\",\n  \"startScene\": \"Title\"\n}\n");
	writeFile(src / top / "templates" / "Demo" / "assets" / "scenes" / "Title.json", "{}");
	writeFile(src / top / "templates" / "Demo" / "build" / "junk.txt", "x");
	fs::path zip = work / (top + ".zip");
	std::string cmd = "C:\\Windows\\System32\\tar.exe -a -cf \"" + u8str(zip) + "\" -C \"" + u8str(src) + "\" " + top;
	auto r = runProcess(cmd);
	if (!r.started || r.exitCode != 0) std::cerr << "tar failed: " << r.output << std::endl;
	return zip;
}

int main()
{
	//%TEMP%(AppData\Local の下)は Claude デスクトップから動かすと振り替わるので、ビルドのフォルダの下を使う
	fs::path work = u8path(GOMI_LAUNCHER_TEST_DIR);
	std::error_code ec;
	fs::remove_all(work, ec);
	fs::create_directories(work);
	fs::path home = work / "home";

	// 番号の比べ方
	CHECK(compareVersions("0.10.0", "0.9.0") > 0);
	CHECK(compareVersions("0.1", "0.1.0") == 0);
	CHECK(compareVersions("1.0.0", "1.0.1") < 0);

	LauncherCore core(home);
	CHECK(core.installedVersions().empty());
	CHECK(core.projects().empty());

	// zip から入れる
	std::string version, error;
	CHECK(core.installZip(makeZip(work, "GOMI-9.9.9"), version, error));
	CHECK(version == "9.9.9");
	CHECK(core.installedVersions() == std::vector<std::string>{ "9.9.9" });
	CHECK(core.templates("9.9.9") == std::vector<std::string>{ "Demo" });
	CHECK(core.installZip(makeZip(work, "GOMI-0.10.0"), version, error));
	CHECK((core.installedVersions() == std::vector<std::string>{ "9.9.9", "0.10.0" })); //新しい順
	// 同じ番号をもう一度・配布物でない zip は断る。作業中のフォルダは残さない
	CHECK(!core.installZip(makeZip(work, "GOMI-9.9.9"), version, error) && error.find("もう入って") != std::string::npos);
	CHECK(!core.installZip(makeZip(work, "other"), version, error));
	CHECK(!core.installZip(work / "none.zip", version, error));
	for (auto& entry : fs::directory_iterator(core.editorsDir())) CHECK(u8str(entry.path().filename())[0] != '.');

	// ひな形から作る(日本語と空白のあるパス)
	fs::path project = work / u8path("プロジェクト") / "My Game";
	CHECK(core.createProject(project, "9.9.9", "Demo", error));
	CHECK(LauncherCore::projectEngine(project) == "9.9.9"); //engine を選んだ番号に書き換える
	CHECK(readFile(project / "project.json").find("\"startScene\": \"Title\"") != std::string::npos);
	CHECK(fs::exists(project / "assets" / "scenes" / "Title.json"));
	CHECK(!fs::exists(project / "build"));
	CHECK(core.projects().size() == 1);
	CHECK(fs::equivalent(u8path(core.projectsDir()), project.parent_path()));
	// 空でないフォルダ・無い版・無いひな形には作らない
	CHECK(!core.createProject(project, "9.9.9", "Demo", error));
	CHECK(!core.createProject(work / "p2", "1.0.0", "Demo", error));
	CHECK(!core.createProject(work / "p3", "9.9.9", "Empty", error));

	// 一覧は launcher.json に残る
	{
		LauncherCore again(home);
		CHECK(again.projects().size() == 1);
		CHECK(fs::equivalent(u8path(again.projects()[0].path), project));
		CHECK(fs::equivalent(u8path(again.projectsDir()), project.parent_path()));
	}

	// 既存のフォルダを足す: project.json が無ければ断る。同じものは重ねない
	CHECK(!core.addProject(work, error));
	CHECK(core.addProject(project, error) && core.projects().size() == 1);

	// 開く: 入っていない版なら missingVersion に番号。engine が無ければ空
	writeFile(project / "project.json", "{\"engine\": \"1.2.3\"}");
	std::string missing;
	CHECK(!core.openProject(project, error, "", &missing) && missing == "1.2.3");
	writeFile(project / "project.json", "{}");
	missing = "x";
	CHECK(!core.openProject(project, error, "", &missing) && missing.empty());
	CHECK(!core.openProject(work / "none", error));

	// ---- 一覧からの編集
	fs::path game = work / "edit" / "Game";
	CHECK(core.createProject(game, "9.9.9", "Demo", error));
	writeFile(game / "assets" / "scenes" / "sub" / "Stage1.json", "{}");
	writeFile(game / "assets" / "ui" / "pause.json", "{}");
	writeFile(game / "assets" / "ui" / "hud" / "life.json", "{}");
	writeFile(game / "assets" / "ui" / "memo.txt", "x");
	CHECK((LauncherCore::projectScenes(game) == std::vector<std::string>{ "Title", "sub/Stage1" }));
	CHECK((LauncherCore::projectUIs(game) == std::vector<std::string>{ "ui/hud/life.json", "ui/pause.json" }));

	// 設定: 項目を書き換え・足し・消す。知らない項目と並びは残す
	writeFile(game / "project.json", "{\n  \"engine\": \"9.9.9\",\n  \"custom\": [1, 2],\n  \"startScene\": \"Title\"\n}\n");
	CHECK(LauncherCore::setProjectValue(game, "engine", "0.10.0", error));
	CHECK(LauncherCore::setProjectValue(game, "pauseUI", "ui/pause.json", error));
	CHECK(LauncherCore::setProjectValue(game, "startScene", "sub/Stage1", error));
	CHECK(LauncherCore::projectEngine(game) == "0.10.0");
	CHECK(LauncherCore::projectValue(game, "pauseUI") == "ui/pause.json");
	{
		std::string text = readFile(game / "project.json");
		CHECK(text.find("\"custom\"") != std::string::npos);
		CHECK(text.find("\"engine\"") < text.find("\"custom\"") && text.find("\"custom\"") < text.find("\"startScene\""));
	}
	CHECK(LauncherCore::setProjectValue(game, "pauseUI", "", error));
	CHECK(readFile(game / "project.json").find("pauseUI") == std::string::npos);
	CHECK(!LauncherCore::setProjectValue(work / "none", "engine", "1", error));
	writeFile(work / "broken" / "project.json", "{ not json");
	CHECK(!LauncherCore::setProjectValue(work / "broken", "engine", "1", error));
	CHECK(readFile(work / "broken" / "project.json") == "{ not json"); //壊れたものは書き換えない

	// 名前に使える・日本語など
	CHECK(LauncherCore::validFolderName("My Game_2"));
	CHECK(!LauncherCore::validFolderName("a/b") && !LauncherCore::validFolderName("a:b") && !LauncherCore::validFolderName("") && !LauncherCore::validFolderName("x."));
	CHECK(LauncherCore::hasNonAscii(u8str(u8path("ゲーム"))) && !LauncherCore::hasNonAscii("Game"));

	// 名前を変える: フォルダと一覧の場所が変わる。同じ名前があれば断る
	size_t before = core.projects().size();
	fs::path renamed;
	CHECK(core.renameProject(game, "Renamed", error, &renamed));
	CHECK(!fs::exists(game) && fs::exists(work / "edit" / "Renamed" / "project.json"));
	CHECK(core.projects().size() == before);
	CHECK(std::any_of(core.projects().begin(), core.projects().end(), [&](auto& p) { return fs::equivalent(u8path(p.path), renamed); }));
	fs::create_directories(work / "edit" / "Taken");
	CHECK(!core.renameProject(renamed, "Taken", error));
	CHECK(!core.renameProject(renamed, "a?b", error));
	game = renamed;

	// 複製: build/ を除き、名前は _copy、_copy2 と増える
	writeFile(game / "build" / "x.obj", "x");
	CHECK(LauncherCore::duplicateName(game) == "Renamed_copy");
	CHECK(core.duplicateProject(game, game.parent_path() / "Renamed_copy", error));
	CHECK(fs::exists(game.parent_path() / "Renamed_copy" / "assets" / "ui" / "hud" / "life.json"));
	CHECK(!fs::exists(game.parent_path() / "Renamed_copy" / "build"));
	CHECK(LauncherCore::duplicateName(game) == "Renamed_copy2");
	CHECK(core.projects().size() == before + 1);
	CHECK(!core.duplicateProject(game, game.parent_path() / "Renamed_copy", error)); //空でない
	CHECK(!core.duplicateProject(game, game / "inner", error));                      //自分の中

	// 一覧から外す(フォルダは残る)
	CHECK(core.removeProject(game.parent_path() / "Renamed_copy", error));
	CHECK(core.projects().size() == before && fs::exists(game.parent_path() / "Renamed_copy"));
	CHECK(!core.removeProject(game.parent_path() / "Renamed_copy", error));
	CHECK(LauncherCore(home).projects().size() == before);

	// ごみ箱: project.json の無いフォルダは消さない(本当にごみ箱へ移すのは手で確かめる)
	fs::create_directories(work / "notproject");
	CHECK(!core.trashProject(work / "notproject", error) && fs::exists(work / "notproject"));
	// フォルダがもう無ければ一覧から外すだけ
	fs::path gone = work / "edit" / "Gone";
	CHECK(core.duplicateProject(game, gone, error));
	fs::remove_all(gone);
	CHECK(core.trashProject(gone, error) && core.projects().size() == before);

	// エクスプローラー・VS Code: 無いフォルダは断る
	CHECK(!LauncherCore::revealInExplorer(work / "none", error));
	CHECK(!LauncherCore::openInVSCode(work / "none", error));

	// ---- バージョンを消す: 使っているプロジェクトが分かる。消すと一覧から消える
	CHECK(LauncherCore::setProjectValue(game, "engine", "0.10.0", error));
	{
		auto using010 = core.projectsUsing("0.10.0");
		CHECK(using010.size() == 1 && fs::equivalent(u8path(using010[0]), game));
	}
	CHECK(core.uninstall("0.10.0", error));
	CHECK((core.installedVersions() == std::vector<std::string>{ "9.9.9" }));
	CHECK(!fs::exists(core.editorDir("0.10.0")));
	CHECK(!core.uninstall("0.10.0", error));
	for (auto& entry : fs::directory_iterator(core.editorsDir())) CHECK(u8str(entry.path().filename())[0] != '.');
	// 使っている版を開こうとすると入っていないと出る
	std::string missing2;
	CHECK(!core.openProject(game, error, "", &missing2) && missing2 == "0.10.0");

	if (failures == 0) std::cout << "LauncherTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
