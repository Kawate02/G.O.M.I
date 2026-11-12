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
