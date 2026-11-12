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
		static const std::vector<std::string> withValue = { "install", "install-zip", "list-templates", "new", "add", "open" };
		static const std::vector<std::string> noValue = { "list-versions", "list-releases", "list-projects", "gh-status", "version" };
		for (size_t i = 0; i < args.size(); i++)
		{
			const std::string& a = args[i];
			if (a == "-h" || a == "--help") { help = true; continue; }
			if (a.rfind("--", 0) != 0) return "不明な引数: " + a;
			std::string name = a.substr(2);
			auto next = [&](std::string& out) -> bool {
				if (i + 1 >= args.size()) return false;
				out = args[++i];
				return true;
			};
			bool isValueCommand = std::find(withValue.begin(), withValue.end(), name) != withValue.end();
			bool isCommand = isValueCommand || std::find(noValue.begin(), noValue.end(), name) != noValue.end();
			if (isCommand)
			{
				if (!command.empty()) return "コマンドは1つだけです: --" + command + " と " + a;
				command = name;
				if (isValueCommand && !next(value)) return a + " に値がありません";
				continue;
			}
			static const std::vector<std::string> options = { "home", "engine", "template", "tab", "frames", "screenshot", "click" };
			if (std::find(options.begin(), options.end(), name) == options.end()) return "不明なオプション: " + a;
			std::string v;
			if (!next(v)) return a + " に値がありません";
			if (name == "home") home = v;
			else if (name == "engine") engine = v;
			else if (name == "template") templateName = v;
			else if (name == "tab") tab = v;
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
		return std::nullopt;
	}
};
