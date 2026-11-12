#pragma once
// ランチャーの起動オプション。コマンド(--install など)は1つだけ
#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#define LAUNCHER_VERSION "0.1.0"

struct LauncherOptions
{
	std::string home;
	std::string command; // "install" など(-- を外したもの)。空なら画面
	std::string value;   // コマンドの引数
	std::string value2;  // 引数を2つ取るコマンド(--rename <dir> <名前> など)の2つ目
	std::string newName; // --duplicate の新しい名前
	bool force = false;  // --uninstall で使っているプロジェクトがあっても消す
	std::string engine;
	std::string templateName;
	bool help = false;

	// 画面の確認用
	int frames = 0; // このコマ数で閉じる(0 なら閉じない)
	std::vector<std::pair<int, std::string>> screenshots;
	std::string tab;
	struct Click { int frame; float x, y; };
	std::vector<Click> clicks; // そのコマで押し、次のコマで離す

	std::optional<std::string> parse(const std::vector<std::string>& args)
	{
		static const std::vector<std::string> withValue = { "install", "install-zip", "list-templates", "new", "add", "open",
			"show", "duplicate", "remove", "trash", "reveal", "code", "uninstall" };
		static const std::vector<std::string> withTwoValues = { "set", "set-engine", "rename" };
		static const std::vector<std::string> noValue = { "list-versions", "list-releases", "list-projects", "gh-status", "version" };
		for (size_t i = 0; i < args.size(); i++)
		{
			const std::string& a = args[i];
			if (a == "-h" || a == "--help") { help = true; continue; }
			if (a == "--force") { force = true; continue; }
			if (a.rfind("--", 0) != 0) return "不明な引数: " + a;
			std::string name = a.substr(2);
			auto next = [&](std::string& out) -> bool {
				if (i + 1 >= args.size()) return false;
				out = args[++i];
				return true;
			};
			bool isValueCommand = std::find(withValue.begin(), withValue.end(), name) != withValue.end();
			bool isTwoValueCommand = std::find(withTwoValues.begin(), withTwoValues.end(), name) != withTwoValues.end();
			bool isCommand = isValueCommand || isTwoValueCommand || std::find(noValue.begin(), noValue.end(), name) != noValue.end();
			if (isCommand)
			{
				if (!command.empty()) return "コマンドは1つだけです: --" + command + " と " + a;
				command = name;
				if ((isValueCommand || isTwoValueCommand) && !next(value)) return a + " に値がありません";
				if (isTwoValueCommand && !next(value2)) return a + " に値が2つ要ります";
				continue;
			}
			static const std::vector<std::string> options = { "home", "engine", "template", "tab", "frames", "screenshot", "click", "name" };
			if (std::find(options.begin(), options.end(), name) == options.end()) return "不明なオプション: " + a;
			std::string v;
			if (!next(v)) return a + " に値がありません";
			if (name == "home") home = v;
			else if (name == "engine") engine = v;
			else if (name == "template") templateName = v;
			else if (name == "tab") tab = v;
			else if (name == "name") newName = v;
			else if (name == "frames")
			{
				try { frames = std::stoi(v); } catch (...) { return "--frames は数です: " + v; }
			}
			else if (name == "screenshot")
			{
				auto colon = v.find(':');
				if (colon == std::string::npos) return "--screenshot は <コマ>:<png> です: " + v;
				try { screenshots.push_back({ std::stoi(v.substr(0, colon)), v.substr(colon + 1) }); }
				catch (...) { return "--screenshot は <コマ>:<png> です: " + v; }
			}
			else if (name == "click")
			{
				Click c{};
				if (sscanf_s(v.c_str(), "%d:%f,%f", &c.frame, &c.x, &c.y) != 3) return "--click は <コマ>:<x>,<y> です: " + v;
				clicks.push_back(c);
			}
			else return "不明なオプション: " + a;
		}
		if (!engine.empty() && command != "new" && command != "open") return "--engine は --new か --open と使います";
		if (!newName.empty() && command != "duplicate") return "--name は --duplicate と使います";
		if (force && command != "uninstall") return "--force は --uninstall と使います";
		return std::nullopt;
	}
};
