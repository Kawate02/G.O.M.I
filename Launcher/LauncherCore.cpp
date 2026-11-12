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

	// build/(ゲームのコードのビルド結果)を除いてコピーする。開いた時にそのバージョンの sdk で作り直す
	static bool copyWithoutBuild(const fs::path& source, const fs::path& dest, std::string& error)
	{
		std::error_code ec;
		for (auto& entry : fs::directory_iterator(source, ec))
		{
			if (entry.path().filename() == "build") continue;
			fs::copy(entry.path(), dest / entry.path().filename(), fs::copy_options::recursive, ec);
			if (ec) { error = "コピーできませんでした: " + u8str(entry.path()) + " (" + ec.message() + ")"; return false; }
		}
		if (ec) { error = "読めませんでした: " + u8str(source) + " (" + ec.message() + ")"; return false; }
		return true;
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

		if (!copyWithoutBuild(source, dest, error)) return false;

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

	// ------------------------------------------------------------ プロジェクトの編集

	static bool readProjectJson(const fs::path& dir, json& j, std::string& error)
	{
		std::ifstream in(dir / "project.json");
		if (!in) { error = "project.json がありません: " + u8str(dir); return false; }
		try { j = json::parse(in); }
		catch (const std::exception& e) { error = "project.json を読めませんでした: " + std::string(e.what()); return false; }
		if (!j.is_object()) { error = "project.json が { } の形ではありません"; return false; }
		return true;
	}

	std::string LauncherCore::projectValue(const fs::path& dir, const std::string& key)
	{
		json j;
		std::string error;
		if (!readProjectJson(dir, j, error) || !j.contains(key) || !j[key].is_string()) return {};
		return j[key].get<std::string>();
	}

	bool LauncherCore::setProjectValue(const fs::path& dir, const std::string& key, const std::string& value, std::string& error)
	{
		if (key.empty()) { error = "項目の名前がありません"; return false; }
		json j;
		if (!readProjectJson(dir, j, error)) return false;
		if (value.empty()) j.erase(key);
		else j[key] = value;
		fs::path tmp = dir / "project.json.tmp";
		{
			std::ofstream out(tmp, std::ios::binary);
			if (!out) { error = "書けませんでした: " + u8str(tmp); return false; }
			out << j.dump(2) << "\n";
		}
		std::error_code ec;
		fs::rename(tmp, dir / "project.json", ec);
		if (ec) { error = "書けませんでした: " + u8str(dir / "project.json") + " (" + ec.message() + ")"; return false; }
		return true;
	}

	// folder の下の .json を、base からの "/" 区切りのパスで並べる
	static std::vector<std::string> listJson(const fs::path& folder, const fs::path& base, bool stripExtension)
	{
		std::vector<std::string> out;
		std::error_code ec;
		for (auto it = fs::recursive_directory_iterator(folder, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
		{
			if (!it->is_regular_file() || it->path().extension() != ".json") continue;
			fs::path rel = fs::relative(it->path(), base, ec);
			if (stripExtension) rel.replace_extension();
			std::string s = u8str(rel);
			std::replace(s.begin(), s.end(), '\\', '/');
			out.push_back(s);
		}
		std::sort(out.begin(), out.end());
		return out;
	}

	std::vector<std::string> LauncherCore::projectScenes(const fs::path& dir)
	{
		return listJson(dir / "assets" / "scenes", dir / "assets" / "scenes", true);
	}

	std::vector<std::string> LauncherCore::projectUIs(const fs::path& dir)
	{
		return listJson(dir / "assets" / "ui", dir / "assets", false);
	}

	bool LauncherCore::validFolderName(const std::string& name)
	{
		if (name.empty() || name == "." || name == "..") return false;
		if (name.find_first_of("\\/:*?\"<>|") != std::string::npos) return false;
		return name.back() != ' ' && name.back() != '.';
	}

	bool LauncherCore::hasNonAscii(const std::string& s)
	{
		return std::any_of(s.begin(), s.end(), [](char c) { return static_cast<unsigned char>(c) >= 0x80; });
	}

	bool LauncherCore::renameProject(const fs::path& dir, const std::string& newName, std::string& error, fs::path* newDir)
	{
		if (!validFolderName(newName)) { error = "フォルダ名に使えない名前です: " + newName; return false; }
		std::error_code ec;
		fs::path abs = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		if (!fs::exists(abs / "project.json")) { error = "project.json がありません: " + u8str(abs); return false; }
		fs::path dest = abs.parent_path() / u8path(newName);
		if (newDir) *newDir = dest;
		if (u8str(abs.filename()) == newName) return true;
		//大文字と小文字だけの違いは同じフォルダ(Windows)なので、そのまま変えてよい
		if (fs::exists(dest) && !samePath(dest, abs)) { error = "同じ名前のフォルダがもうあります: " + u8str(dest); return false; }
		ProjectEntry* entry = findProject(abs);
		fs::rename(abs, dest, ec);
		if (ec)
		{
			error = "名前を変えられませんでした: " + u8str(abs) + "(エディタ・エクスプローラー・VS Code などで開いていると変えられません。閉じてからもう一度)";
			return false;
		}
		if (entry) entry->path = u8str(dest);
		else projectList.insert(projectList.begin(), { u8str(dest), "" });
		return save(error);
	}

	bool LauncherCore::removeProject(const fs::path& dir, std::string& error)
	{
		auto it = std::find_if(projectList.begin(), projectList.end(), [&](auto& p) { return samePath(u8path(p.path), dir); });
		if (it == projectList.end()) { error = "一覧にありません: " + u8str(dir); return false; }
		projectList.erase(it);
		return save(error);
	}

	bool LauncherCore::trashProject(const fs::path& dir, std::string& error)
	{
		std::error_code ec;
		fs::path abs = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		auto it = std::find_if(projectList.begin(), projectList.end(), [&](auto& p) { return samePath(u8path(p.path), abs); });
		if (fs::exists(abs))
		{
			if (!fs::exists(abs / "project.json")) { error = "G.O.M.I のプロジェクト(project.json のあるフォルダ)ではないので消しません: " + u8str(abs); return false; }
			//SHFileOperation の pFrom は 0 が2つで終わる
			std::wstring from = abs.wstring();
			from.push_back(L'\0');
			SHFILEOPSTRUCTW op{};
			op.wFunc = FO_DELETE;
			op.pFrom = from.c_str();
			//ごみ箱が無いドライブでは、完全に消す前に確認を出す(FOF_WANTNUKEWARNING)
			op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_WANTNUKEWARNING;
			int r = SHFileOperationW(&op);
			if (r != 0 || op.fAnyOperationsAborted || fs::exists(abs))
			{
				error = "ごみ箱へ移せませんでした: " + u8str(abs) + "(エディタなどで開いていると移せません。閉じてからもう一度)";
				return false;
			}
		}
		if (it != projectList.end()) projectList.erase(it);
		return save(error);
	}

	std::string LauncherCore::duplicateName(const fs::path& dir)
	{
		std::error_code ec;
		fs::path abs = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		std::string base = u8str(abs.filename()) + "_copy";
		std::string name = base;
		for (int n = 2; fs::exists(abs.parent_path() / u8path(name)); n++) name = base + std::to_string(n);
		return name;
	}

	bool LauncherCore::duplicateProject(const fs::path& dir, const fs::path& dest, std::string& error)
	{
		std::error_code ec;
		fs::path source = fs::weakly_canonical(fs::absolute(dir, ec), ec);
		if (!fs::exists(source / "project.json")) { error = "project.json がありません: " + u8str(source); return false; }
		fs::path target = fs::weakly_canonical(fs::absolute(dest, ec), ec);
		if (!validFolderName(u8str(target.filename()))) { error = "フォルダ名に使えない名前です: " + u8str(target.filename()); return false; }
		if (fs::exists(target) && !fs::is_empty(target, ec)) { error = "フォルダが空ではありません: " + u8str(target); return false; }
		//自分の中へのコピーは終わらない
		std::wstring s = source.wstring() + L"\\", t = target.wstring() + L"\\";
		if (_wcsnicmp(t.c_str(), s.c_str(), s.size()) == 0) { error = "プロジェクトの中には複製できません"; return false; }
		fs::create_directories(target, ec);
		if (ec) { error = "フォルダを作れませんでした: " + u8str(target); return false; }
		if (!copyWithoutBuild(source, target, error)) return false;
		if (!findProject(target)) projectList.insert(projectList.begin(), { u8str(target), "" });
		return save(error);
	}

	bool LauncherCore::revealInExplorer(const fs::path& dir, std::string& error)
	{
		if (!fs::is_directory(dir)) { error = "フォルダがありません: " + u8str(dir); return false; }
		auto r = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
		if (r <= 32) { error = "エクスプローラーで開けませんでした: " + u8str(dir); return false; }
		return true;
	}

	fs::path LauncherCore::findVSCode()
	{
		//PATH の code(<入れた所>/bin/code.cmd)→ ユーザー用の既定の場所 → 全員用の既定の場所
		wchar_t buffer[MAX_PATH];
		if (SearchPathW(nullptr, L"code.cmd", nullptr, MAX_PATH, buffer, nullptr) > 0)
		{
			fs::path exe = fs::path(buffer).parent_path().parent_path() / "Code.exe";
			if (fs::exists(exe)) return exe;
		}
		for (auto id : { FOLDERID_UserProgramFiles, FOLDERID_ProgramFiles })
		{
			fs::path folder = knownFolder(id);
			if (!folder.empty() && fs::exists(folder / "Microsoft VS Code" / "Code.exe")) return folder / "Microsoft VS Code" / "Code.exe";
		}
		return {};
	}

	bool LauncherCore::openInVSCode(const fs::path& dir, std::string& error)
	{
		if (!fs::is_directory(dir)) { error = "フォルダがありません: " + u8str(dir); return false; }
		fs::path exe = findVSCode();
		if (exe.empty())
		{
			error = "VS Code が見つかりません。入れるには、コマンドで winget install Microsoft.VisualStudioCode を実行して、ランチャーを開き直してください。";
			return false;
		}
		return startDetached(exe, { u8str(dir) }, dir, error);
	}

	// ------------------------------------------------------------ バージョンを消す

	std::vector<std::string> LauncherCore::projectsUsing(const std::string& version) const
	{
		std::vector<std::string> out;
		for (auto& p : projectList)
		{
			if (projectEngine(u8path(p.path)) == version) out.push_back(p.path);
		}
		return out;
	}

	bool LauncherCore::uninstall(const std::string& version, std::string& error)
	{
		if (!isInstalled(version)) { error = version + " は入っていません"; return false; }
		//先に作業中の名前へ移す。エディタが動いていると exe が使われていて移せないので、途中まで消えて壊れることが無い
		fs::path removing = editorsDir() / u8path(".remove-" + version);
		std::error_code ec;
		fs::remove_all(removing, ec);
		fs::rename(editorDir(version), removing, ec);
		if (ec)
		{
			error = "消せませんでした: " + u8str(editorDir(version)) + "(エディタ " + version + " を開いていると消せません。閉じてからもう一度)";
			return false;
		}
		fs::remove_all(removing, ec); //残っても .remove-* は一覧に出ず、次に同じ版を消す時に消える
		return true;
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
