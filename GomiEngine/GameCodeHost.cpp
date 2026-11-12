#include "GameCodeHost.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <shellapi.h>

#include <cctype>
#include <chrono>
#include <fstream>
#include <set>

namespace fs = std::filesystem;

namespace
{
	bool isSource(const fs::path& p)
	{
		auto ext = p.extension().string();
		return ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".inl";
	}

	//ビルドの出力は UTF-8 のことも ANSI(CP932)のこともあるので、UTF-8 として読めなければ ANSI とみなす
	std::string toUtf8(const std::string& bytes)
	{
		if (bytes.empty()) return bytes;
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0) > 0) return bytes;
		int wn = MultiByteToWideChar(CP_ACP, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
		std::wstring w(wn, L'\0');
		MultiByteToWideChar(CP_ACP, 0, bytes.data(), (int)bytes.size(), w.data(), wn);
		int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, nullptr, 0, nullptr, nullptr);
		std::string out(n, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, out.data(), n, nullptr, nullptr);
		return out;
	}

	std::wstring quote(const std::wstring& s) { return L"\"" + s + L"\""; }
	std::wstring widen(const std::string& s) { return fs::path(s).wstring(); }
	fs::path fromUtf8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

	/// <summary>コマンドを窓を出さずに動かし、標準出力とエラー出力をまとめて返す</summary>
	bool runProcess(const std::wstring& commandLine, std::string& output, DWORD& exitCode)
	{
		SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
		HANDLE readPipe = nullptr, writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = writePipe;
		si.hStdError = writePipe;
		si.hStdInput = nullptr;
		PROCESS_INFORMATION pi{};
		std::wstring cmd = commandLine;
		BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
		CloseHandle(writePipe);
		if (!ok)
		{
			CloseHandle(readPipe);
			return false;
		}
		std::string bytes;
		char buffer[4096];
		DWORD n = 0;
		while (ReadFile(readPipe, buffer, sizeof(buffer), &n, nullptr) && n > 0) bytes.append(buffer, n);
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(readPipe);
		output = toUtf8(bytes);
		return true;
	}

	std::vector<std::string> errorLines(const std::string& output)
	{
		std::vector<std::string> lines;
		std::set<std::string> seen;
		size_t start = 0;
		while (start < output.size())
		{
			size_t end = output.find('\n', start);
			if (end == std::string::npos) end = output.size();
			std::string line = output.substr(start, end - start);
			start = end + 1;
			while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
			if (line.find(": error") == std::string::npos && line.find("error MSB") == std::string::npos) continue;
			//MSBuild が後ろに付ける " [....vcxproj]" と先頭の空白を取る
			size_t bracket = line.rfind(" [");
			if (bracket != std::string::npos && line.back() == ']') line.erase(bracket);
			size_t first = line.find_first_not_of(' ');
			if (first != std::string::npos) line.erase(0, first);
			if (seen.insert(line).second) lines.push_back(line);
		}
		return lines;
	}
}

GameCodeHost::Config GameCodeHost::Config::forProject(const fs::path& projectDir, const fs::path& sdkDir, const std::string& buildConfig)
{
	Config c;
	c.projectDir = projectDir;
	c.sourceDir = projectDir / "game";
	c.sdkDir = sdkDir;
	c.buildDir = projectDir / "build";
	c.buildConfig = buildConfig;
	c.builtDll = c.buildDir / "bin" / GameModule::DLL_NAME;
	c.liveDir = c.buildDir / "live";
	return c;
}

fs::path GameCodeHost::findCMake()
{
	wchar_t found[MAX_PATH];
	if (SearchPathW(nullptr, L"cmake.exe", nullptr, MAX_PATH, found, nullptr) > 0) return fs::path(found);
	//Visual Studio の場所は vswhere(VS のインストーラが置く)で引く
	wchar_t programFiles[MAX_PATH];
	DWORD n = GetEnvironmentVariableW(L"ProgramFiles(x86)", programFiles, MAX_PATH);
	if (n == 0 || n >= MAX_PATH) return {};
	fs::path vswhere = fs::path(programFiles) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
	std::error_code ec;
	if (!fs::exists(vswhere, ec)) return {};
	std::string output;
	DWORD exitCode = 0;
	if (!runProcess(quote(vswhere.wstring()) + L" -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath", output, exitCode) || exitCode != 0) return {};
	while (!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' ')) output.pop_back();
	if (output.empty()) return {};
	fs::path cmake = fromUtf8(output) / "Common7" / "IDE" / "CommonExtensions" / "Microsoft" / "CMake" / "CMake" / "bin" / "cmake.exe";
	return fs::exists(cmake, ec) ? cmake : fs::path();
}

GameCodeHost::GameCodeHost(Config config) : config(std::move(config))
{
	if (!enabled()) return;
	//前回までのコピーを消す(別のエディタが使っているものは消せないので残る)
	std::error_code ec;
	if (fs::exists(this->config.liveDir, ec))
	{
		for (auto& entry : fs::directory_iterator(this->config.liveDir, ec)) fs::remove(entry.path(), ec);
	}
	stamps = scan();
}

GameCodeHost::~GameCodeHost()
{
	if (worker.joinable()) worker.join();
}

bool GameCodeHost::enabled() const
{
	std::error_code ec;
	return !config.sourceDir.empty() && fs::is_directory(config.sourceDir, ec)
		&& fs::is_regular_file(config.sdkDir / "game" / "CMakeLists.txt", ec);
}

std::map<fs::path, fs::file_time_type> GameCodeHost::scan() const
{
	std::map<fs::path, fs::file_time_type> found;
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(config.sourceDir, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
	{
		if (!it->is_regular_file(ec) || !isSource(it->path())) continue;
		found[it->path()] = it->last_write_time(ec);
	}
	return found;
}

bool GameCodeHost::needsFirstBuild() const
{
	std::error_code ec;
	return enabled() && !stamps.empty() && !fs::exists(config.builtDll, ec);
}

bool GameCodeHost::builtChanged() const
{
	std::error_code ec;
	if (!fs::exists(config.builtDll, ec)) return false;
	return !gameModule.isLoaded() || fs::last_write_time(config.builtDll, ec) != loadedStamp;
}

bool GameCodeHost::copyAndLoad(std::string& error)
{
	std::error_code ec;
	fs::create_directories(config.liveDir, ec);
	fs::path copy = config.liveDir / ("GomiGame_" + std::to_string(++copyNumber) + ".dll");
	loadedStamp = fs::last_write_time(config.builtDll, ec);
	if (!fs::copy_file(config.builtDll, copy, fs::copy_options::overwrite_existing, ec))
	{
		error = "cannot copy " + config.builtDll.string() + ": " + ec.message();
		return false;
	}
	return gameModule.load(copy, error);
}

bool GameCodeHost::loadBuilt(std::string& error)
{
	std::error_code ec;
	if (!fs::exists(config.builtDll, ec)) return false;
	return copyAndLoad(error);
}

void GameCodeHost::poll(double now)
{
	if (!enabled() || now - lastPoll < 0.5) return;
	lastPoll = now;
	if (buildAgain && !building) startBuild(); //ビルド中に保存されていた
	auto current = scan();
	if (current != stamps)
	{
		stamps = std::move(current);
		changed = true; //続けて保存されることがあるので、次に見て変わっていなければビルドする
		return;
	}
	if (!changed) return;
	changed = false;
	if (building) buildAgain = true;
	else startBuild();
}

void GameCodeHost::startBuild()
{
	if (building) return;
	if (worker.joinable()) worker.join();
	building = true;
	buildAgain = false;
	worker = std::thread([this]() {
		auto begin = std::chrono::steady_clock::now();
		BuildResult r;
		DWORD exitCode = 0;
		bool started = true;
		std::vector<std::wstring> commands;
		if (config.cmake.empty()) config.cmake = findCMake();
		if (config.cmake.empty())
		{
			r.errors.push_back("cmake が見つかりません(PATH にも Visual Studio にもありません。Visual Studio の「C++ によるデスクトップ開発」を入れてください)");
			r.output = r.errors.back();
			std::lock_guard lock(resultMutex);
			result = std::move(r);
			building = false;
			return;
		}
		std::error_code ec;
		//別のエンジン(sdk)で作ったビルドのフォルダは作り直す(cmake は元と違う -S を受け付けない)
		auto cache = config.buildDir / "CMakeCache.txt";
		if (fs::exists(cache, ec))
		{
			std::ifstream in(cache);
			std::string line, home;
			while (std::getline(in, line))
			{
				if (line.rfind("CMAKE_HOME_DIRECTORY:INTERNAL=", 0) == 0) { home = line.substr(30); break; }
			}
			in.close();
			if (!fs::equivalent(fromUtf8(home), config.sdkDir / "game", ec))
			{
				fs::remove(cache, ec);
				fs::remove_all(config.buildDir / "CMakeFiles", ec);
			}
		}
		if (!fs::exists(cache, ec))
		{
			commands.push_back(quote(config.cmake.wstring()) + L" -S " + quote((config.sdkDir / "game").wstring())
				+ L" -B " + quote(config.buildDir.wstring()) + L" -DGOMI_PROJECT=" + quote(config.projectDir.generic_wstring()));
		}
		auto command = [this](const std::string& target) {
			return quote(config.cmake.wstring()) + L" --build " + quote(config.buildDir.wstring())
				+ L" --config " + widen(config.buildConfig) + L" --target " + widen(target);
		};
		//Visual Studio の構成では、ファイルを足した時の作り直し(ZERO_CHECK)を先に別に回す。
		//同じビルドの中だと、MSBuild が作り直す前のプロジェクトを読んでいて、足したファイルが入らない
		if (commands.empty() && fs::exists(config.buildDir / "ZERO_CHECK.vcxproj", ec)) commands.push_back(command("ZERO_CHECK"));
		commands.push_back(command(config.target));
		for (auto& cmd : commands)
		{
			std::string output;
			if (!runProcess(cmd, output, exitCode))
			{
				started = false;
				break;
			}
			r.output += output;
			if (exitCode != 0) break;
		}
		if (!started)
		{
			r.output += "cannot start cmake (error " + std::to_string(GetLastError()) + ")";
			r.errors.push_back(r.output);
		}
		else
		{
			r.ok = exitCode == 0;
			r.errors = errorLines(r.output);
			if (!r.ok && r.errors.empty()) r.errors.push_back("build failed (exit code " + std::to_string(exitCode) + ")");
		}
		r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
		{
			std::lock_guard lock(resultMutex);
			result = std::move(r);
		}
		building = false;
	});
}

std::optional<GameCodeHost::BuildResult> GameCodeHost::takeResult()
{
	std::optional<BuildResult> r;
	{
		std::lock_guard lock(resultMutex);
		r.swap(result);
	}
	return r;
}

namespace
{
	bool isIdentifier(const std::string& s)
	{
		if (s.empty() || std::isdigit(static_cast<unsigned char>(s[0]))) return false;
		for (char c : s)
		{
			if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
		}
		return true;
	}

	std::string replaceAll(std::string text, const std::string& from, const std::string& to)
	{
		for (size_t pos = text.find(from); pos != std::string::npos; pos = text.find(from, pos + to.size())) text.replace(pos, from.size(), to);
		return text;
	}

	const char* COMPONENT_HEADER = R"(#pragma once
// @Name@ コンポーネント: (何を表すかを書く)
// 項目を足したら @Name@Component.cpp の GOMI_REGISTER にも .field で並べる(エディタのインスペクタ・シーンファイルに出る)
#include "Vec3.h"

struct @Name@Component
{
	float value = 0;
};
)";

	const char* COMPONENT_SOURCE = R"(#include "GameCode.h"
#include "@Name@Component.h"

GOMI_REGISTER
{
	// .field は作る時に決める値、.runtime はゲーム中に System が変える値(エディタでは読むだけ)
	components.add<@Name@Component>("@Name@")
		.field("value", &@Name@Component::value);
}
)";

	const char* SYSTEM_SOURCE = R"(// @Name@System: (何をするかを書く)
// シーンの System タブの「System を追加」で並べると、上から順に毎フレーム動く
#include "GameCode.h"
#include "TransformComponent.h"

class @Name@System : public ISystem
{
public:
	void initialize(World& world) override {}

	void update(World& world, int frameCount, float deltaTime) override
	{
		auto& cm = world.getComponentManager();
		// 例: Transform を持つ Entity を全部回す。自分の Component は "XxxComponent.h" を include して each<XxxComponent, ...>
		cm.each<TransformComponent>([&](Entity e) {
			TransformComponent& transform = cm.getComponentArray<TransformComponent>().getComponent(e);
			(void)transform;
		});
	}
};

GOMI_REGISTER
{
	systems.add<@Name@System>("@Name@System");
}
)";
}

std::vector<fs::path> GameCodeHost::createTemplate(TemplateKind kind, const std::string& rawName, std::string& error) const
{
	std::string name = rawName;
	const std::string suffix = kind == TemplateKind::Component ? "Component" : "System";
	if (name.size() > suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) name.erase(name.size() - suffix.size());
	if (!isIdentifier(name))
	{
		error = "\"" + rawName + "\" は名前に使えません(英字で始まる英数字と _ だけ)";
		return {};
	}
	std::vector<std::pair<fs::path, const char*>> files;
	if (kind == TemplateKind::Component)
	{
		files.push_back({ config.sourceDir / (name + "Component.h"), COMPONENT_HEADER });
		files.push_back({ config.sourceDir / (name + "Component.cpp"), COMPONENT_SOURCE });
	}
	else files.push_back({ config.sourceDir / (name + "System.cpp"), SYSTEM_SOURCE });
	std::error_code ec;
	for (auto& [path, text] : files)
	{
		if (fs::exists(path, ec))
		{
			error = path.filename().string() + " はもうあります";
			return {};
		}
	}
	fs::create_directories(config.sourceDir, ec);
	std::vector<fs::path> made;
	for (auto& [path, text] : files)
	{
		std::ofstream out(path, std::ios::binary);
		out << replaceAll(text, "@Name@", name);
		if (!out)
		{
			error = "cannot write " + path.string();
			return {};
		}
		made.push_back(path);
	}
	//Component は登録の .cpp より型の .h を先に開く
	return made;
}

bool GameCodeHost::openInCodeEditor(const fs::path& folder, const fs::path& file, std::string& error)
{
	wchar_t found[MAX_PATH];
	if (SearchPathW(nullptr, L"code.cmd", nullptr, MAX_PATH, found, nullptr) > 0)
	{
		std::wstring cmd = L"cmd.exe /c \"" + quote(found) + L" " + quote(folder.wstring());
		if (!file.empty()) cmd += L" -g " + quote(file.wstring());
		cmd += L"\"";
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return true;
		}
	}
	//VS Code が無い: ファイルなら既定のアプリ、無ければフォルダをエクスプローラーで開く
	const fs::path& target = file.empty() ? folder : file;
	auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", target.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
	if (result <= 32)
	{
		error = "cannot open " + target.string();
		return false;
	}
	return true;
}

bool GameCodeHost::swapToBuilt(std::string& error)
{
	previousCopy = gameModule.path();
	gameModule.unload();
	return copyAndLoad(error);
}

void GameCodeHost::revert()
{
	gameModule.unload();
	if (previousCopy.empty()) return;
	std::string error;
	gameModule.load(previousCopy, error);
}
