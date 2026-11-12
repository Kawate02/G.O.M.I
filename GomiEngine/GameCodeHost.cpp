#include "GameCodeHost.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <chrono>
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
	return !config.sourceDir.empty() && fs::is_directory(config.sourceDir, ec);
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
	auto command = [this](const std::string& target) {
		return quote(config.cmake.wstring()) + L" --build " + quote(config.buildDir.wstring())
			+ L" --config " + widen(config.buildConfig) + L" --target " + widen(target);
	};
	std::vector<std::wstring> commands;
	//Visual Studio の構成では、ファイルを足した時の作り直し(ZERO_CHECK)を先に別に回す。
	//同じビルドの中だと、MSBuild が作り直す前のプロジェクトを読んでいて、足したファイルが入らない
	std::error_code ec;
	if (fs::exists(config.buildDir / "ZERO_CHECK.vcxproj", ec)) commands.push_back(command("ZERO_CHECK"));
	commands.push_back(command(config.target));
	worker = std::thread([this, commands]() {
		auto begin = std::chrono::steady_clock::now();
		BuildResult r;
		DWORD exitCode = 0;
		bool started = true;
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
