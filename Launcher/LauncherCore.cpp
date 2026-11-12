#include "LauncherCore.h"
#include "json.hpp"
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>

namespace launcher
{
	using json = nlohmann::ordered_json;

	fs::path u8path(const std::string& s)
	{
		return fs::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
	}

	std::string u8str(const fs::path& p)
	{
		auto u = p.u8string();
		return std::string(u.begin(), u.end());
	}

	static std::wstring widen(const std::string& s)
	{
		if (s.empty()) return {};
		int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
		std::wstring w(n, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
		return w;
	}

	static std::string quote(const std::string& s)
	{
		return "\"" + s + "\"";
	}

	int compareVersions(const std::string& a, const std::string& b)
	{
		std::istringstream sa(a), sb(b);
		std::string pa, pb;
		while (true)
		{
			bool ha = static_cast<bool>(std::getline(sa, pa, '.'));
			bool hb = static_cast<bool>(std::getline(sb, pb, '.'));
			if (!ha && !hb) return 0;
			long na = 0, nb = 0;
			try { na = ha ? std::stol(pa) : 0; } catch (...) {}
			try { nb = hb ? std::stol(pb) : 0; } catch (...) {}
			if (na != nb) return na < nb ? -1 : 1;
		}
	}

	ProcessResult runProcess(const std::string& commandLine, const fs::path& workDir)
	{
		ProcessResult result;
		SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
		HANDLE readPipe = nullptr, writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return result;
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = writePipe;
		si.hStdError = writePipe;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		PROCESS_INFORMATION pi{};
		std::wstring cmd = widen(commandLine);
		std::wstring dir = workDir.empty() ? std::wstring() : workDir.wstring();
		BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
			dir.empty() ? nullptr : dir.c_str(), &si, &pi);
		CloseHandle(writePipe);
		if (!ok)
		{
			CloseHandle(readPipe);
			return result;
		}
		result.started = true;
		char buffer[4096];
		DWORD n = 0;
		while (ReadFile(readPipe, buffer, sizeof(buffer), &n, nullptr) && n > 0) result.output.append(buffer, n);
		CloseHandle(readPipe);
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD code = 0;
		GetExitCodeProcess(pi.hProcess, &code);
		result.exitCode = static_cast<int>(code);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return result;
	}

	bool startDetached(const fs::path& exe, const std::vector<std::string>& args, const fs::path& workDir, std::string& error)
	{
		std::string commandLine = quote(u8str(exe));
		for (auto& a : args) commandLine += " " + quote(a);
		std::wstring cmd = widen(commandLine);
		std::wstring dir = workDir.wstring();
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		//エディタは標準エラーを自分のログの欄に出すので、黒い窓は出さない
		if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, dir.c_str(), &si, &pi))
		{
			error = "起動できませんでした: " + u8str(exe) + " (エラー " + std::to_string(GetLastError()) + ")";
			return false;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// Git Bash の tar が PATH の先にあっても、Windows 標準の tar.exe(zip を展開できる)を使う
	static std::string systemTar()
	{
		wchar_t buffer[MAX_PATH];
		UINT n = GetSystemDirectoryW(buffer, MAX_PATH);
		return u8str(fs::path(std::wstring(buffer, n)) / "tar.exe");
	}

	static std::string nowText()
	{
		std::time_t t = std::time(nullptr);
		std::tm tm{};
		localtime_s(&tm, &t);
		char buffer[32];
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
		return buffer;
	}

	static fs::path knownFolder(REFKNOWNFOLDERID id)
	{
		PWSTR path = nullptr;
		fs::path result;
		if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &path))) result = path;
		CoTaskMemFree(path);
		return result;
	}

	static std::string trimLines(const std::string& s)
	{
		std::string out = s;
		while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) out.pop_back();
		return out;
	}

	// ------------------------------------------------------------

	LauncherCore::LauncherCore(fs::path home) : homeDir(std::move(home))
	{
		homeDir.make_preferred();
		load();
	}

	fs::path LauncherCore::defaultHome()
	{
		//%LOCALAPPDATA% は Claude デスクトップから動かすと書き込みが Packages の LocalCache に振り替わるので使わない
		return knownFolder(FOLDERID_Profile) / "GOMI";
	}

	std::vector<std::string> LauncherCore::installedVersions() const
	{
		std::vector<std::string> versions;
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(editorsDir(), ec))
		{
			if (!entry.is_directory()) continue;
			std::string name = u8str(entry.path().filename());
			if (name.empty() || name[0] == '.') continue; //.download-* などの作業中のフォルダ
			if (!fs::exists(entry.path() / "GomiEditor.exe")) continue;
			versions.push_back(name);
		}
		std::sort(versions.begin(), versions.end(), [](auto& a, auto& b) { return compareVersions(a, b) > 0; });
		return versions;
	}

	bool LauncherCore::isInstalled(const std::string& version) const
	{
		return !version.empty() && fs::exists(editorDir(version) / "GomiEditor.exe");
	}

	std::vector<std::string> LauncherCore::templates(const std::string& version) const
	{
		std::vector<std::string> names;
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(editorDir(version) / "templates", ec))
		{
			if (entry.is_directory() && fs::exists(entry.path() / "project.json")) names.push_back(u8str(entry.path().filename()));
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	GhStatus LauncherCore::checkGh() const
	{
		GhStatus status;
		auto version = runProcess("gh --version");
		status.installed = version.started && version.exitCode == 0;
		if (!status.installed)
		{
			status.message = "GitHub CLI(gh)が見つかりません。入れるには、コマンドで winget install GitHub.cli を実行して、ランチャーを開き直してください。"
				"落とした zip(GOMI-<番号>-windows-x64.zip)からなら gh 無しでも入れられます。";
			return status;
		}
		auto auth = runProcess("gh auth status");
		status.loggedIn = auth.started && auth.exitCode == 0;
		if (!status.loggedIn)
			status.message = "gh が GitHub にログインしていません。コマンドで gh auth login を実行してから「再確認」を押してください。"
				"(配布物は Private のリポジトリ " + std::string(RELEASE_REPO) + " にあるので、見られるアカウントでログインします)";
		return status;
	}

	bool LauncherCore::listReleases(std::vector<Release>& out, std::string& error) const
	{
		out.clear();
		auto r = runProcess("gh release list -R " + std::string(RELEASE_REPO) + " --json tagName,publishedAt --limit 100");
		if (!r.started) { error = "gh を起動できませんでした"; return false; }
		if (r.exitCode != 0) { error = "バージョンの一覧を取れませんでした: " + trimLines(r.output); return false; }
		try
		{
			for (auto& item : json::parse(r.output))
			{
				std::string tag = item.value("tagName", "");
				if (tag.size() < 2 || tag[0] != 'v') continue;
				std::string published = item.value("publishedAt", "");
				out.push_back({ tag.substr(1), published.substr(0, 10) });
			}
		}
		catch (const std::exception& e)
		{
			error = std::string("gh の出力を読めませんでした: ") + e.what();
			return false;
		}
		std::sort(out.begin(), out.end(), [](auto& a, auto& b) { return compareVersions(a.version, b.version) > 0; });
		return true;
	}

	bool LauncherCore::install(const std::string& version, std::string& error, const Progress& progress)
	{
		if (isInstalled(version)) { error = version + " はもう入っています"; return false; }
		fs::path download = editorsDir() / u8path(".download-" + version);
		std::error_code ec;
		fs::remove_all(download, ec);
		fs::create_directories(download, ec);
		if (ec) { error = "フォルダを作れませんでした: " + u8str(download); return false; }

		if (progress) progress(version + " を落としています…");
		auto r = runProcess("gh release download v" + version + " -R " + std::string(RELEASE_REPO) + " -p \"*.zip\" -D " + quote(u8str(download)));
		if (!r.started || r.exitCode != 0)
		{
			error = "落とせませんでした: " + (r.started ? trimLines(r.output) : std::string("gh を起動できませんでした"));
			fs::remove_all(download, ec);
			return false;
		}
		fs::path zip;
		for (auto& entry : fs::directory_iterator(download, ec))
		{
			if (entry.path().extension() == ".zip") zip = entry.path();
		}
		if (zip.empty())
		{
			error = "v" + version + " に zip がありませんでした";
			fs::remove_all(download, ec);
			return false;
		}
		std::string got;
		bool ok = installZip(zip, got, error, progress);
		if (ok && got != version)
		{
			//zip の中の番号がタグと違う(配布物の作り間違い)。入れたものは残し、知らせる
			error = "v" + version + " の zip の中身は " + got + " でした";
			ok = false;
		}
		fs::remove_all(download, ec);
		return ok;
	}

	bool LauncherCore::installZip(const fs::path& zip, std::string& version, std::string& error, const Progress& progress)
	{
		if (!fs::exists(zip)) { error = "zip がありません: " + u8str(zip); return false; }
		fs::path staging = editorsDir() / u8path(".extract-" + std::to_string(GetCurrentProcessId()));
		std::error_code ec;
		fs::remove_all(staging, ec);
		fs::create_directories(staging, ec);
		if (ec) { error = "フォルダを作れませんでした: " + u8str(staging); return false; }

		if (progress) progress("展開しています…");
		auto r = runProcess(quote(systemTar()) + " -xf " + quote(u8str(zip)) + " -C " + quote(u8str(staging)));
		if (!r.started || r.exitCode != 0)
		{
			error = "展開できませんでした: " + (r.started ? trimLines(r.output) : std::string("tar.exe を起動できませんでした"));
			fs::remove_all(staging, ec);
			return false;
		}
		bool ok = placeExtracted(staging, "", version, error);
		fs::remove_all(staging, ec);
		if (ok && progress) progress(version + " を入れました");
		return ok;
	}

	bool LauncherCore::placeExtracted(const fs::path& staging, const std::string& expectVersion, std::string& version, std::string& error)
	{
		fs::path top;
		int count = 0;
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(staging, ec))
		{
			count++;
			if (entry.is_directory()) top = entry.path();
		}
		std::string name = top.empty() ? "" : u8str(top.filename());
		if (count != 1 || name.rfind("GOMI-", 0) != 0)
		{
			error = "G.O.M.I の配布物の zip ではありません(中が GOMI-<番号>/ の1フォルダになっていません)";
			return false;
		}
		version = name.substr(5);
		if (!expectVersion.empty() && version != expectVersion)
		{
			error = "zip の中身は " + version + " でした(" + expectVersion + " のはず)";
			return false;
		}
		if (!fs::exists(top / "GomiEditor.exe"))
		{
			error = "zip に GomiEditor.exe がありません";
			return false;
		}
		if (isInstalled(version))
		{
			error = version + " はもう入っています";
			return false;
		}
		fs::path dest = editorDir(version);
		fs::remove_all(dest, ec); //前に途中で止まった残り
		fs::rename(top, dest, ec);
		if (ec)
		{
			error = "置けませんでした: " + u8str(dest) + " (" + ec.message() + ")";
			return false;
		}
		return true;
	}

	// ------------------------------------------------------------

	static bool samePath(const fs::path& a, const fs::path& b)
	{
		std::error_code ec;
		if (fs::exists(a, ec) && fs::exists(b, ec)) return fs::equivalent(a, b, ec);
		return _wcsicmp(fs::weakly_canonical(a, ec).wstring().c_str(), fs::weakly_canonical(b, ec).wstring().c_str()) == 0;
	}

	ProjectEntry* LauncherCore::findProject(const fs::path& dir)
	{
		for (auto& p : projectList)
		{
			if (samePath(u8path(p.path), dir)) return &p;
		}
		return nullptr;
	}

	bool LauncherCore::addProject(const fs::path& dir, std::string& error)
	{
		std::error_code ec;
		fs::path abs = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		if (!fs::exists(abs / "project.json"))
		{
			error = "project.json がありません: " + u8str(abs);
			return false;
		}
		if (findProject(abs)) return true;
		projectList.insert(projectList.begin(), { u8str(abs), "" });
		return save(error);
	}

	std::string LauncherCore::projectEngine(const fs::path& dir)
	{
		std::ifstream in(dir / "project.json");
		if (!in) return {};
		try
		{
			json j = json::parse(in);
			return j.value("engine", "");
		}
		catch (...)
		{
			return {};
		}
	}

	bool LauncherCore::createProject(const fs::path& dir, const std::string& version, const std::string& templateName, std::string& error)
	{
		if (!isInstalled(version)) { error = version + " は入っていません"; return false; }
		fs::path source = editorDir(version) / "templates" / u8path(templateName);
		if (templateName.empty() || !fs::exists(source / "project.json"))
		{
			error = version + " にひな形 " + templateName + " がありません";
			return false;
		}
		std::error_code ec;
		fs::path dest = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		if (fs::exists(dest) && !fs::is_empty(dest, ec))
		{
			error = "フォルダが空ではありません: " + u8str(dest);
			return false;
		}
		fs::create_directories(dest, ec);
		if (ec) { error = "フォルダを作れませんでした: " + u8str(dest); return false; }

		//build/(ゲームのコードのビルド結果)はコピーしない。開いた時にそのバージョンの sdk で作り直す
		for (auto& entry : fs::directory_iterator(source, ec))
		{
			if (entry.path().filename() == "build") continue;
			fs::copy(entry.path(), dest / entry.path().filename(), fs::copy_options::recursive, ec);
			if (ec) { error = "コピーできませんでした: " + u8str(entry.path()) + " (" + ec.message() + ")"; return false; }
		}

		//使うバージョンを選んだものにする(ひな形の engine は配布した時の番号)
		fs::path projectJson = dest / "project.json";
		try
		{
			json j;
			{
				std::ifstream in(projectJson);
				j = json::parse(in);
			}
			j["engine"] = version;
			std::ofstream out(projectJson, std::ios::binary);
			out << j.dump(2) << "\n";
		}
		catch (const std::exception& e)
		{
			error = std::string("project.json を書けませんでした: ") + e.what();
			return false;
		}

		defaultProjectsDir = u8str(dest.parent_path());
		if (!findProject(dest)) projectList.insert(projectList.begin(), { u8str(dest), "" });
		return save(error);
	}

	bool LauncherCore::openProject(const fs::path& dir, std::string& error, const std::string& engineOverride, std::string* missingVersion)
	{
		if (!fs::exists(dir / "project.json"))
		{
			error = "project.json がありません: " + u8str(dir);
			return false;
		}
		std::string version = engineOverride.empty() ? projectEngine(dir) : engineOverride;
		if (version.empty())
		{
			error = "project.json に engine(使うバージョン)がありません。開くバージョンを選んでください";
			if (missingVersion) missingVersion->clear();
			return false;
		}
		if (!isInstalled(version))
		{
			error = "エンジン " + version + " が入っていません";
			if (missingVersion) *missingVersion = version;
			return false;
		}
		std::error_code ec;
		fs::path abs = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		if (!startDetached(editorDir(version) / "GomiEditor.exe", { "--project", u8str(abs) }, editorDir(version), error)) return false;

		ProjectEntry* entry = findProject(abs);
		if (!entry)
		{
			projectList.insert(projectList.begin(), { u8str(abs), "" });
			entry = &projectList.front();
		}
		entry->lastOpened = nowText();
		return save(error);
	}

	// ------------------------------------------------------------

	void LauncherCore::load()
	{
		projectList.clear();
		defaultProjectsDir = u8str(knownFolder(FOLDERID_Documents) / "GOMI Projects");
		std::ifstream in(homeDir / "launcher.json");
		if (!in) return;
		try
		{
			json j = json::parse(in);
			for (auto& p : j.value("projects", json::array()))
				projectList.push_back({ p.value("path", ""), p.value("lastOpened", "") });
			if (j.contains("projectsDir")) defaultProjectsDir = j["projectsDir"].get<std::string>();
		}
		catch (...)
		{
			//壊れていたら空から始める(次に保存した時に書き直す)
		}
	}

	bool LauncherCore::save(std::string& error) const
	{
		json j;
		j["projectsDir"] = defaultProjectsDir;
		j["projects"] = json::array();
		for (auto& p : projectList) j["projects"].push_back({ { "path", p.path }, { "lastOpened", p.lastOpened } });
		std::error_code ec;
		fs::create_directories(homeDir, ec);
		fs::path tmp = homeDir / "launcher.json.tmp";
		{
			std::ofstream out(tmp, std::ios::binary);
			if (!out) { error = "書けませんでした: " + u8str(tmp); return false; }
			out << j.dump(2) << "\n";
		}
		fs::rename(tmp, homeDir / "launcher.json", ec);
		if (ec) { error = "書けませんでした: " + u8str(homeDir / "launcher.json") + " (" + ec.message() + ")"; return false; }
		return true;
	}
}
