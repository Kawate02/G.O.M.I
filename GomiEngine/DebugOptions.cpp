#include "DebugOptions.h"
#include <unordered_map>

namespace
{
	const std::unordered_map<std::string, InputKey> keyNames = {
		{ "ESCAPE", InputKey::KEY_ESCAPE },
		{ "SPACE", InputKey::KEY_SPACE },
		{ "RETURN", InputKey::KEY_RETURN },
		{ "LSHIFT", InputKey::KEY_LSHIFT },
		{ "LEFT", InputKey::KEY_LEFT },
		{ "RIGHT", InputKey::KEY_RIGHT },
		{ "UP", InputKey::KEY_UP },
		{ "DOWN", InputKey::KEY_DOWN },
		{ "W", InputKey::KEY_W },
		{ "A", InputKey::KEY_A },
		{ "S", InputKey::KEY_S },
		{ "D", InputKey::KEY_D },
	};

	// "<frame>:<rest>" または "<start>-<end>:<rest>" を分ける
	bool splitFrame(const std::string& arg, int& start, int& end, std::string& rest)
	{
		auto colon = arg.find(':');
		if (colon == std::string::npos) return false;
		std::string frames = arg.substr(0, colon);
		rest = arg.substr(colon + 1);
		try
		{
			auto dash = frames.find('-');
			start = std::stoi(frames.substr(0, dash));
			end = dash == std::string::npos ? start : std::stoi(frames.substr(dash + 1));
		}
		catch (...)
		{
			return false;
		}
		return end >= start;
	}
}

std::optional<std::string> DebugOptions::parse(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		std::string opt = argv[i];
		if (i + 1 >= argc) return "missing value for " + opt;
		std::string value = argv[++i];
		int start = 0, end = 0;
		std::string rest;

		if (opt == "--world") startWorld = value;
		else if (opt == "--frames")
		{
			try { maxFrames = std::stoi(value); }
			catch (...) { return "bad --frames: " + value; }
		}
		else if (opt == "--fixed-dt")
		{
			try { fixedDeltaTime = 1.0f / std::stof(value); }
			catch (...) { return "bad --fixed-dt: " + value; }
			if (!(fixedDeltaTime > 0)) return "bad --fixed-dt: " + value;
		}
		else if (opt == "--screenshot")
		{
			if (!splitFrame(value, start, end, rest) || rest.empty()) return "bad --screenshot: " + value;
			screenshots.push_back({ start, rest });
		}
		else if (opt == "--key")
		{
			if (!splitFrame(value, start, end, rest)) return "bad --key: " + value;
			auto it = keyNames.find(rest);
			if (it == keyNames.end()) return "unknown key: " + rest;
			keys.push_back({ start, end, it->second });
		}
		else if (opt == "--click")
		{
			auto comma = value.find(',');
			if (!splitFrame(value, start, end, rest) || (comma = rest.find(',')) == std::string::npos) return "bad --click: " + value;
			try { clicks.push_back({ start, { std::stof(rest.substr(0, comma)), std::stof(rest.substr(comma + 1)) } }); }
			catch (...) { return "bad --click: " + value; }
		}
		else return "unknown option: " + opt;
	}
	return std::nullopt;
}

void DebugOptions::pressAt(InputState& state, InputKey key, int frame, int start, int end) const
{
	int i = static_cast<int>(key);
	if (frame >= start && frame <= end)
	{
		state.key[i] = 1;
		state.deltakey[i] = frame == start ? 0 : 1;
	}
	else if (frame == end + 1)
	{
		state.key[i] = 0;
		state.deltakey[i] = 1;
	}
}

void DebugOptions::apply(int frame, InputState& state) const
{
	for (auto& k : keys)
	{
		pressAt(state, k.key, frame, k.start, k.end);
	}
	for (auto& c : clicks)
	{
		// 押した次のフレームで離す(UIButton は離した瞬間をクリックとみなす)
		if (frame == c.frame || frame == c.frame + 1) state.mouse.position = c.pos;
		pressAt(state, InputKey::MOUSE_LEFT, frame, c.frame, c.frame);
	}
}

std::vector<std::string> DebugOptions::screenshotsAt(int frame) const
{
	std::vector<std::string> result;
	for (auto& s : screenshots)
	{
		if (s.frame == frame) result.push_back(s.path);
	}
	return result;
}
