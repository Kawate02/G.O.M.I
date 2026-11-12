#pragma once
// ランチャーの中身(画面無し)。入れたバージョン・プロジェクトの一覧・Releases から入れる・プロジェクトを作る・開く。
// 画面(LauncherApp)とコマンド(LauncherMain の --install など)の両方がこれを呼ぶ。
// 文字列のパスは全部 UTF-8。
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace launcher
{
	namespace fs = std::filesystem;

	// UTF-8 の文字列とパスの行き来(std::filesystem::path(std::string) は ANSI として読むので日本語が化ける)
	fs::path u8path(const std::string& s);
	std::string u8str(const fs::path& p);

	// "0.10.2" と "0.9.0" を数で比べる(a < b なら負)
	int compareVersions(const std::string& a, const std::string& b);

	// コマンドを動かして標準出力と標準エラーを受け取る(窓は出さない)
	struct ProcessResult
	{
		bool started = false;
		int exitCode = -1;
		std::string output;
	};
	ProcessResult runProcess(const std::string& commandLine, const fs::path& workDir = {});
	// 待たずに起動する(エディタを開く)
	bool startDetached(const fs::path& exe, const std::vector<std::string>& args, const fs::path& workDir, std::string& error);

	struct ProjectEntry
	{
		std::string path;       // プロジェクトのフォルダ(project.json のある所)
		std::string lastOpened; // "2026-11-12 21:05"。開いたことが無ければ空
	};

	// gh の状態。入れる(Releases を見る)にだけ要る
	struct GhStatus
	{
		bool installed = false;
		bool loggedIn = false;
		std::string message; // 使えない時の理由と直し方
		bool ok() const { return installed && loggedIn; }
	};

	struct Release
	{
		std::string version; // "0.1.0"(タグ v0.1.0 から v を外したもの)
		std::string published;
	};

	class LauncherCore
	{
	public:
		static constexpr const char* RELEASE_REPO = "Kawate02/G.O.M.I-releases";

		// home: 置き場所(既定は %USERPROFILE%/GOMI)。中に editors/<番号>/ と launcher.json
		explicit LauncherCore(fs::path home);
		static fs::path defaultHome();

		const fs::path& home() const { return homeDir; }
		fs::path editorsDir() const { return homeDir / "editors"; }
		fs::path editorDir(const std::string& version) const { return editorsDir() / u8path(version); }

		// ---- 入れたバージョン
		std::vector<std::string> installedVersions() const; // 新しい順
		bool isInstalled(const std::string& version) const;
		std::vector<std::string> templates(const std::string& version) const; // そのバージョンの templates/ の中のフォルダ名

		// ---- 入れる。progress には進み具合の短い文を渡す
		using Progress = std::function<void(const std::string&)>;
		GhStatus checkGh() const;
		bool listReleases(std::vector<Release>& out, std::string& error) const; // 新しい順
		bool install(const std::string& version, std::string& error, const Progress& progress = {});
		// 落とした zip(中は GOMI-<番号>/ 1つ)から入れる。入れた番号を version に返す
		bool installZip(const fs::path& zip, std::string& version, std::string& error, const Progress& progress = {});

		// ---- プロジェクト
		const std::vector<ProjectEntry>& projects() const { return projectList; }
		std::string projectsDir() const { return defaultProjectsDir; } // 新しいプロジェクトを作る既定の親フォルダ
		// 一覧に足す(project.json のあるフォルダだけ)。もう有れば何もしない
		bool addProject(const fs::path& dir, std::string& error);
		// templates/<template> をコピーして dir に作り、engine を version にして一覧に足す。dir は無いか空のフォルダ
		bool createProject(const fs::path& dir, const std::string& version, const std::string& templateName, std::string& error);
		// project.json の engine(無ければ空)
		static std::string projectEngine(const fs::path& dir);
		// engine のバージョンのエディタで開く。engineOverride を渡すとそのバージョンで開く(project.json は変えない)
		// 入っていない時は false で、missingVersion にその番号
		bool openProject(const fs::path& dir, std::string& error, const std::string& engineOverride = {}, std::string* missingVersion = nullptr);

		// ---- プロジェクトの編集(一覧の行から)
		// project.json の key を value にする(value が空なら項目を消す)。ほかの項目と並びはそのまま
		static bool setProjectValue(const fs::path& dir, const std::string& key, const std::string& value, std::string& error);
		static std::string projectValue(const fs::path& dir, const std::string& key);
		// 選べるシーン(assets/scenes/ の .json、拡張子無し)と UI(assets/ の下の ui/ の .json、"ui/pause.json" の形)
		static std::vector<std::string> projectScenes(const fs::path& dir);
		static std::vector<std::string> projectUIs(const fs::path& dir);
		// 名前に使えない文字・日本語など(エディタ 0.1.0 が開けない)の判定
		static bool validFolderName(const std::string& name);
		static bool hasNonAscii(const std::string& s);
		// フォルダ名を変える。一覧の場所も直す。newDir に変えた後のフォルダ
		bool renameProject(const fs::path& dir, const std::string& newName, std::string& error, fs::path* newDir = nullptr);
		// 一覧から外す(フォルダはそのまま)
		bool removeProject(const fs::path& dir, std::string& error);
		// ごみ箱へ移して一覧から外す
		bool trashProject(const fs::path& dir, std::string& error);
		// 複製の既定の名前(<名前>_copy、有れば _copy2 …)
		static std::string duplicateName(const fs::path& dir);
		// build/ を除いてコピーし、一覧に足す。dest は無いか空のフォルダ
		bool duplicateProject(const fs::path& dir, const fs::path& dest, std::string& error);
		// エクスプローラー・VS Code で開く
		static bool revealInExplorer(const fs::path& dir, std::string& error);
		static fs::path findVSCode(); // 見つからなければ空
		static bool openInVSCode(const fs::path& dir, std::string& error);

		// ---- 入れたバージョンを消す
		// 一覧のプロジェクトのうち、project.json の engine が version のもの
		std::vector<std::string> projectsUsing(const std::string& version) const;
		// 完全に消す(Releases から入れ直せるのでごみ箱には入れない)。そのエディタを開いたままだと消せない
		bool uninstall(const std::string& version, std::string& error);

		// launcher.json を読み直す・書く
		void load();
		bool save(std::string& error) const;

	private:
		fs::path homeDir;
		std::vector<ProjectEntry> projectList;
		std::string defaultProjectsDir;

		bool placeExtracted(const fs::path& staging, const std::string& expectVersion, std::string& version, std::string& error);
		ProjectEntry* findProject(const fs::path& dir);
	};
}
