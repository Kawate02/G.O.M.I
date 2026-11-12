// G.O.M.I ランチャー。引数無しなら画面、コマンドの引数があればそれだけ行って終わる(画面と同じ LauncherCore を呼ぶ)
#include "LauncherCore.h"
#include "LauncherOptions.h"
#include <windows.h>
#include <shellapi.h>
#include <iostream>

using namespace launcher;

int runGui(LauncherCore& core, const LauncherOptions& options); // LauncherApp.cpp

static const char* USAGE =
	"使い方: GomiLauncher [--home <dir>] [コマンド]\n"
	"  (コマンド無し)                         画面を開く\n"
	"  --list-versions                        入れたバージョン\n"
	"  --list-releases                        Releases にあるバージョン(gh を使う)\n"
	"  --install <番号>                       Releases から入れる(gh を使う)\n"
	"  --install-zip <zip>                    落とした GOMI-<番号>-windows-x64.zip から入れる\n"
	"  --list-templates <番号>                そのバージョンのひな形\n"
	"  --new <dir> --engine <番号> [--template <名>]  ひな形から新しいプロジェクトを作る(既定のひな形は Demo)\n"
	"  --add <dir>                            既存のプロジェクトを一覧に足す\n"
	"  --list-projects                        プロジェクトの一覧(engine と、入っているか)\n"
	"  --open <dir> [--engine <番号>]          project.json の engine のエディタで開く\n"
	"  --show <dir>                           project.json の設定と、選べるシーン・UI\n"
	"  --set <dir> <項目>=<値>                 project.json の項目を直す(値が空なら消す。例: startScene=Title)\n"
	"  --set-engine <dir> <番号>               使うバージョンを変える(入れた版だけ)\n"
	"  --rename <dir> <新しい名前>             フォルダ名を変える\n"
	"  --duplicate <dir> [--name <名前>]       build/ を除いて複製する(既定の名前は <名前>_copy)\n"
	"  --remove <dir>                         一覧から外す(フォルダは残す)\n"
	"  --trash <dir>                          ごみ箱へ移して一覧から外す\n"
	"  --reveal <dir>                         エクスプローラーで開く\n"
	"  --code <dir>                           VS Code で開く\n"
	"  --uninstall <番号> [--force]            入れたバージョンを消す(使っているプロジェクトがあれば --force が要る)\n"
	"  --gh-status                            gh が使えるか\n"
	"  --version                              ランチャーのバージョン\n"
	"画面の確認用: --frames <n>・--screenshot <コマ>:<png>・--click <コマ>:<x>,<y>・--tab projects|versions|new\n"
	"--home の既定は %USERPROFILE%\\GOMI(入れたバージョンは editors/<番号>/、一覧は launcher.json)";

static std::vector<std::string> utf8Args()
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++)
	{
		std::wstring w = argv[i];
		int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
		std::string s(n, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
		args.push_back(s);
	}
	LocalFree(argv);
	return args;
}

static int fail(const std::string& message)
{
	std::cerr << message << std::endl;
	return 1;
}

static int runCommand(LauncherCore& core, const LauncherOptions& o)
{
	std::string error;
	auto progress = [](const std::string& s) { std::cout << s << std::endl; };
	if (o.command == "list-versions")
	{
		for (auto& v : core.installedVersions()) std::cout << v << "\t" << u8str(core.editorDir(v)) << "\n";
		return 0;
	}
	if (o.command == "gh-status")
	{
		auto gh = core.checkGh();
		if (gh.ok()) { std::cout << "gh: 使えます" << std::endl; return 0; }
		return fail(gh.message);
	}
	if (o.command == "list-releases")
	{
		std::vector<Release> releases;
		if (!core.listReleases(releases, error)) return fail(error);
		for (auto& r : releases) std::cout << r.version << "\t" << r.published << "\t" << (core.isInstalled(r.version) ? "入れ済み" : "") << "\n";
		return 0;
	}
	if (o.command == "install")
	{
		if (!core.install(o.value, error, progress)) return fail(error);
		return 0;
	}
	if (o.command == "install-zip")
	{
		std::string version;
		if (!core.installZip(u8path(o.value), version, error, progress)) return fail(error);
		return 0;
	}
	if (o.command == "list-templates")
	{
		if (!core.isInstalled(o.value)) return fail(o.value + " は入っていません");
		for (auto& t : core.templates(o.value)) std::cout << t << "\n";
		return 0;
	}
	if (o.command == "new")
	{
		if (o.engine.empty()) return fail("--new には --engine <番号> が要ります");
		std::string tmpl = o.templateName;
		if (tmpl.empty())
		{
			auto list = core.templates(o.engine);
			tmpl = std::find(list.begin(), list.end(), "Demo") != list.end() ? "Demo" : (list.empty() ? "" : list.front());
		}
		if (!core.createProject(u8path(o.value), o.engine, tmpl, error)) return fail(error);
		std::cout << "作りました: " << o.value << "(エンジン " << o.engine << "、ひな形 " << tmpl << ")" << std::endl;
		return 0;
	}
	if (o.command == "add")
	{
		if (!core.addProject(u8path(o.value), error)) return fail(error);
		return 0;
	}
	if (o.command == "list-projects")
	{
		for (auto& p : core.projects())
		{
			std::string engine = LauncherCore::projectEngine(u8path(p.path));
			std::string state = engine.empty() ? "engine無し" : (core.isInstalled(engine) ? "" : "未インストール");
			std::cout << p.path << "\t" << engine << "\t" << state << "\t" << p.lastOpened << "\n";
		}
		return 0;
	}
	if (o.command == "open")
	{
		std::string missing;
		if (!core.openProject(u8path(o.value), error, o.engine, &missing))
		{
			if (!missing.empty()) error += "(GomiLauncher --install " + missing + " で入れるか、--engine で別のバージョンを指定してください)";
			return fail(error);
		}
		return 0;
	}
	fs::path dir = u8path(o.value);
	if (o.command == "show")
	{
		if (!fs::exists(dir / "project.json")) return fail("project.json がありません: " + o.value);
		for (const char* key : { "engine", "startScene", "pauseUI" })
			std::cout << key << "\t" << LauncherCore::projectValue(dir, key) << "\n";
		std::cout << "シーン:";
		for (auto& s : LauncherCore::projectScenes(dir)) std::cout << " " << s;
		std::cout << "\nUI:";
		for (auto& s : LauncherCore::projectUIs(dir)) std::cout << " " << s;
		std::cout << std::endl;
		return 0;
	}
	if (o.command == "set")
	{
		auto eq = o.value2.find('=');
		if (eq == std::string::npos || eq == 0) return fail("--set は <項目>=<値> です: " + o.value2);
		if (!LauncherCore::setProjectValue(dir, o.value2.substr(0, eq), o.value2.substr(eq + 1), error)) return fail(error);
		return 0;
	}
	if (o.command == "set-engine")
	{
		if (!core.isInstalled(o.value2)) return fail(o.value2 + " は入っていません(GomiLauncher --install " + o.value2 + " で入れてください)");
		if (!LauncherCore::setProjectValue(dir, "engine", o.value2, error)) return fail(error);
		return 0;
	}
	if (o.command == "rename")
	{
		fs::path renamed;
		if (!core.renameProject(dir, o.value2, error, &renamed)) return fail(error);
		std::cout << "変えました: " << u8str(renamed) << std::endl;
		if (LauncherCore::hasNonAscii(u8str(renamed)))
			std::cerr << "注意: パスに日本語などが入っています。エディタ 0.1.0 はこのプロジェクトを開けません" << std::endl;
		return 0;
	}
	if (o.command == "duplicate")
	{
		std::string name = o.newName.empty() ? LauncherCore::duplicateName(dir) : o.newName;
		fs::path dest = fs::absolute(dir).parent_path() / u8path(name);
		if (!core.duplicateProject(dir, dest, error)) return fail(error);
		std::cout << "複製しました: " << u8str(dest) << std::endl;
		return 0;
	}
	if (o.command == "remove")
	{
		if (!core.removeProject(dir, error)) return fail(error);
		return 0;
	}
	if (o.command == "trash")
	{
		if (!core.trashProject(dir, error)) return fail(error);
		std::cout << "ごみ箱へ移しました: " << o.value << std::endl;
		return 0;
	}
	if (o.command == "reveal")
	{
		if (!LauncherCore::revealInExplorer(dir, error)) return fail(error);
		return 0;
	}
	if (o.command == "code")
	{
		if (!LauncherCore::openInVSCode(dir, error)) return fail(error);
		return 0;
	}
	if (o.command == "uninstall")
	{
		auto users = core.projectsUsing(o.value);
		if (!users.empty() && !o.force)
		{
			std::cerr << o.value << " を使っているプロジェクトがあります(消すと開く時に入れ直しか別の版を選ぶことになります):\n";
			for (auto& p : users) std::cerr << "  " << p << "\n";
			return fail("それでも消すなら --force を付けてください");
		}
		if (!core.uninstall(o.value, error)) return fail(error);
		std::cout << o.value << " を消しました" << std::endl;
		return 0;
	}
	return fail("不明なコマンド: " + o.command);
}

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	LauncherOptions options;
	if (auto error = options.parse(utf8Args()))
	{
		std::cerr << *error << "\n" << USAGE << std::endl;
		return 2;
	}
	if (options.help) { std::cout << USAGE << std::endl; return 0; }
	if (options.command == "version") { std::cout << LAUNCHER_VERSION << std::endl; return 0; }

	LauncherCore core(options.home.empty() ? LauncherCore::defaultHome() : u8path(options.home));
	if (!options.command.empty()) return runCommand(core, options);

	//エクスプローラーや星界から開いた時に出る黒い窓は閉じる(コマンドプロンプトから開いた時は残す)
	DWORD processes[2];
	if (options.frames <= 0 && GetConsoleProcessList(processes, 2) == 1) FreeConsole();
	return runGui(core, options);
}
