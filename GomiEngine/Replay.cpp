#include "Replay.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace
{
	const char* keyName(InputKey key)
	{
		return enumToName(key);
	}

	InputKey keyFromName(const std::string& name)
	{
		auto key = enumFromName<InputKey>(name);
		if (!key) throw std::runtime_error("unknown key \"" + name + "\"");
		return *key;
	}

	Json eventToJson(const Replay::Event& e)
	{
		Json j = Json::object();
		j["frame"] = e.frame;
		if (e.mouse) j["mouse"] = ::toJson(*e.mouse);
		if (!e.press.empty())
		{
			Json a = Json::array();
			for (auto k : e.press) a.push_back(keyName(k));
			j["press"] = a;
		}
		if (!e.release.empty())
		{
			Json a = Json::array();
			for (auto k : e.release) a.push_back(keyName(k));
			j["release"] = a;
		}
		if (e.wheel) j["wheel"] = e.wheel;
		return j;
	}

	void writeText(const fs::path& path, const std::string& text)
	{
		if (path.has_parent_path()) fs::create_directories(path.parent_path());
		std::ofstream f(path, std::ios::binary);
		if (!f) throw std::runtime_error("cannot write " + path.generic_string());
		f << text;
	}

	void diffRec(const Json& a, const Json& b, const std::string& path, std::vector<std::string>& out, size_t limit, size_t& count)
	{
		if (a == b) return;
		auto add = [&](const std::string& line) {
			count++;
			if (out.size() < limit) out.push_back(line);
		};
		if (a.is_object() && b.is_object())
		{
			for (auto& [k, v] : a.items())
			{
				std::string p = path.empty() ? k : path + "." + k;
				if (!b.contains(k)) add(p + ": " + v.dump() + " -> (none)");
				else diffRec(v, b[k], p, out, limit, count);
			}
			for (auto& [k, v] : b.items())
			{
				if (!a.contains(k)) add((path.empty() ? k : path + "." + k) + ": (none) -> " + v.dump());
			}
			return;
		}
		if (a.is_array() && b.is_array())
		{
			size_t n = std::min(a.size(), b.size());
			for (size_t i = 0; i < n; i++) diffRec(a[i], b[i], path + "[" + std::to_string(i) + "]", out, limit, count);
			if (a.size() != b.size()) add(path + ": length " + std::to_string(a.size()) + " -> " + std::to_string(b.size()));
			return;
		}
		add(path + ": " + a.dump() + " -> " + b.dump());
	}
}

// ---- Replay

Json Replay::toJson() const
{
	Json j = Json::object();
	j["scene"] = scene;
	j["fps"] = fps;
	j["seed"] = seed;
	j["length"] = length;
	j["checkpoints"] = checkpoints;
	Json input = Json::array();
	for (auto& e : events) input.push_back(eventToJson(e));
	j["input"] = input;
	return j;
}

Replay Replay::fromJson(const Json& j)
{
	if (!j.is_object()) throw std::runtime_error("replay: expected an object");
	for (auto& [k, v] : j.items())
	{
		if (k != "scene" && k != "fps" && k != "seed" && k != "length" && k != "checkpoints" && k != "input")
			throw std::runtime_error("replay: unknown key \"" + k + "\"");
	}
	Replay r;
	r.scene = j.value("scene", std::string());
	r.fps = j.value("fps", 60);
	r.seed = j.value("seed", 1u);
	r.length = j.value("length", 0);
	if (r.fps <= 0) throw std::runtime_error("replay: fps must be positive");
	if (r.length <= 0) throw std::runtime_error("replay: length must be positive");
	for (auto& c : j.value("checkpoints", Json::array()))
	{
		int f = c.get<int>();
		if (f < 1 || f > r.length) throw std::runtime_error("replay: checkpoint " + std::to_string(f) + " is out of 1.." + std::to_string(r.length));
		r.checkpoints.push_back(f);
	}
	int lastFrame = 0;
	for (auto& ej : j.value("input", Json::array()))
	{
		Event e;
		for (auto& [k, v] : ej.items())
		{
			if (k == "frame") e.frame = v.get<int>();
			else if (k == "mouse") e.mouse = Vec2(v.at(0).get<float>(), v.at(1).get<float>());
			else if (k == "press") for (auto& n : v) e.press.push_back(keyFromName(n.get<std::string>()));
			else if (k == "release") for (auto& n : v) e.release.push_back(keyFromName(n.get<std::string>()));
			else if (k == "wheel") e.wheel = v.get<int>();
			else throw std::runtime_error("replay input: unknown key \"" + k + "\"");
		}
		if (e.frame < 1 || e.frame < lastFrame) throw std::runtime_error("replay input: frames must start at 1 and not go back (" + std::to_string(e.frame) + ")");
		lastFrame = e.frame;
		r.events.push_back(std::move(e));
	}
	return r;
}

Replay Replay::load(const fs::path& path)
{
	std::ifstream f(path, std::ios::binary);
	if (!f) throw std::runtime_error("cannot open " + path.generic_string());
	try
	{
		return fromJson(Json::parse(f));
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(path.generic_string() + ": " + e.what());
	}
}

void Replay::save(const fs::path& path) const
{
	//入力は1フレーム1行にして、差分で読みやすくする
	Json j = toJson();
	std::ostringstream s;
	s << "{\n";
	for (const char* key : { "scene", "fps", "seed", "length", "checkpoints" })
	{
		s << "  \"" << key << "\": " << j[key].dump() << ",\n";
	}
	s << "  \"input\": [";
	for (size_t i = 0; i < events.size(); i++)
	{
		s << (i ? ",\n    " : "\n    ") << eventToJson(events[i]).dump();
	}
	s << (events.empty() ? "]\n" : "\n  ]\n") << "}\n";
	writeText(path, s.str());
}

fs::path Replay::expectedDir(const fs::path& replayPath)
{
	fs::path p = replayPath;
	return p.replace_extension();
}

// ---- 記録

ReplayRecorder::ReplayRecorder(const std::string& scene, int fps, uint32_t seed)
{
	replay.scene = scene;
	replay.fps = fps;
	replay.seed = seed;
}

void ReplayRecorder::record(int frame, const InputState& state)
{
	Replay::Event e;
	e.frame = frame;
	for (auto& [key, name] : EnumNames<InputKey>::list())
	{
		int i = static_cast<int>(key);
		bool now = state.key[i] != 0;
		bool before = first ? false : last.key[i] != 0;
		if (now && !before) e.press.push_back(key);
		if (!now && before) e.release.push_back(key);
	}
	if (first || state.mouse.position.x != last.mouse.position.x || state.mouse.position.y != last.mouse.position.y) e.mouse = state.mouse.position;
	e.wheel = state.mouse.deltaWheel;
	if (!e.press.empty() || !e.release.empty() || e.mouse || e.wheel) replay.events.push_back(std::move(e));
	last = state;
	first = false;
}

Replay ReplayRecorder::finish(int length, int checkpointEvery) const
{
	Replay r = replay;
	r.length = length;
	r.checkpoints.clear();
	if (checkpointEvery > 0)
	{
		for (int f = checkpointEvery; f < length; f += checkpointEvery) r.checkpoints.push_back(f);
	}
	if (length > 0) r.checkpoints.push_back(length);
	return r;
}

// ---- 再生

void ReplayPlayer::apply(int frame, InputState& out)
{
	for (int i = 0; i < 256; i++) state.deltakey[i] = state.key[i];
	Vec2 before = state.mouse.position;
	state.mouse.deltaWheel = 0;
	while (next < replay.events.size() && replay.events[next].frame <= frame)
	{
		auto& e = replay.events[next++];
		for (auto k : e.press) state.key[static_cast<int>(k)] = 1;
		for (auto k : e.release) state.key[static_cast<int>(k)] = 0;
		if (e.mouse) state.mouse.position = *e.mouse;
		state.mouse.deltaWheel += e.wheel;
	}
	state.mouse.totalWheel += state.mouse.deltaWheel;
	state.mouse.deltaPosition = started ? state.mouse.position - before : Vec2(0, 0);
	started = true;
	out = state;
}

// ---- 比べる

ReplayCheck::ReplayCheck(fs::path expectedDir, fs::path outDir, Mode mode) : expected(std::move(expectedDir)), out(std::move(outDir)), mode(mode)
{
}

std::string ReplayCheck::fileStem(int frame)
{
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%05d", frame);
	return buf;
}

const ReplayCheckResult& ReplayCheck::checkpoint(int frame, const Json& state, const std::function<bool(const std::string&)>& capture)
{
	ReplayCheckResult r;
	r.frame = frame;
	std::string stem = fileStem(frame);
	//数値は保存した文字列を読み直した値で比べる(float を double にした時の端数で違わないように)
	Json actual = Json::parse(state.dump());
	std::string stateText = actual.dump(2) + "\n";

	if (mode == Mode::Update)
	{
		writeText(expected / (stem + ".json"), stateText);
		r.actualImage = (expected / (stem + ".png")).generic_string();
		if (!capture(r.actualImage)) r.imageMissing = true;
		results.push_back(std::move(r));
		return results.back();
	}

	writeText(out / (stem + ".json"), stateText);
	r.actualImage = (out / (stem + ".png")).generic_string();
	bool captured = capture(r.actualImage);

	fs::path expectedState = expected / (stem + ".json");
	std::ifstream f(expectedState, std::ios::binary);
	if (!f) r.stateMissing = true;
	else
	{
		try { r.stateDiffs = jsonDiff(Json::parse(f), actual); }
		catch (const std::exception& e) { r.stateDiffs.push_back(std::string("cannot read expected state: ") + e.what()); }
	}

	std::string diffPath = (out / (stem + "_diff.png")).generic_string();
	std::error_code ec;
	fs::remove(diffPath, ec);
	long long pixels = captured ? compareImages((expected / (stem + ".png")).generic_string(), r.actualImage, diffPath) : -1;
	if (pixels == -1) r.imageMissing = true;
	else if (pixels == -2) r.sizeMismatch = true;
	else
	{
		r.pixelDiffs = pixels;
		if (pixels > 0) r.diffImage = diffPath;
	}
	results.push_back(std::move(r));
	return results.back();
}

bool ReplayCheck::passed(const Replay& replay) const
{
	if (results.size() < replay.checkpoints.size()) return false;
	if (mode == Mode::Update) return true;
	return std::all_of(results.begin(), results.end(), [](const ReplayCheckResult& r) { return r.ok(); });
}

std::vector<std::string> ReplayCheck::report(const Replay& replay, const std::string& name) const
{
	std::vector<std::string> lines;
	std::string head = "replay " + name + ": ";
	if (mode == Mode::Update)
	{
		for (auto& r : results) lines.push_back(head + "frame " + std::to_string(r.frame) + " saved as expected" + (r.imageMissing ? " (no image)" : ""));
		if (results.size() < replay.checkpoints.size()) lines.push_back(head + "stopped before all checkpoints (" + std::to_string(results.size()) + " of " + std::to_string(replay.checkpoints.size()) + ")");
		lines.push_back(head + "expected updated in " + expected.generic_string());
		return lines;
	}
	int failed = 0;
	for (auto& r : results)
	{
		if (r.ok())
		{
			lines.push_back(head + "frame " + std::to_string(r.frame) + " ok");
			continue;
		}
		failed++;
		lines.push_back(head + "frame " + std::to_string(r.frame) + " DIFFERS");
		if (r.stateMissing) lines.push_back("  state: no expected file (run with --replay-update to create)");
		for (auto& d : r.stateDiffs) lines.push_back("  state " + d);
		if (r.imageMissing) lines.push_back("  image: no expected image or cannot capture");
		else if (r.sizeMismatch) lines.push_back("  image: size differs");
		else if (r.pixelDiffs) lines.push_back("  image: " + std::to_string(r.pixelDiffs) + " pixels differ (" + r.diffImage + ")");
	}
	if (results.size() < replay.checkpoints.size())
	{
		failed++;
		lines.push_back(head + "stopped before all checkpoints (" + std::to_string(results.size()) + " of " + std::to_string(replay.checkpoints.size()) + ")");
	}
	lines.push_back(head + (failed ? "FAILED" : "PASSED") + " (" + std::to_string(results.size()) + " checkpoints, output in " + out.generic_string() + ")");
	return lines;
}

std::vector<std::string> ReplayCheck::jsonDiff(const Json& expected, const Json& actual, size_t limit)
{
	std::vector<std::string> out;
	size_t count = 0;
	diffRec(expected, actual, "", out, limit, count);
	if (count > out.size()) out.push_back("... and " + std::to_string(count - out.size()) + " more");
	return out;
}

long long ReplayCheck::compareImages(const std::string& expectedPath, const std::string& actualPath, const std::string& diffPath)
{
	int w1, h1, c1, w2, h2, c2;
	unsigned char* a = stbi_load(expectedPath.c_str(), &w1, &h1, &c1, 4);
	unsigned char* b = stbi_load(actualPath.c_str(), &w2, &h2, &c2, 4);
	long long result;
	if (!a || !b) result = -1;
	else if (w1 != w2 || h1 != h2) result = -2;
	else
	{
		result = 0;
		size_t n = static_cast<size_t>(w1) * h1;
		std::vector<unsigned char> diff(n * 4);
		for (size_t i = 0; i < n; i++)
		{
			bool same = std::equal(a + i * 4, a + i * 4 + 4, b + i * 4);
			if (!same) result++;
			//違う所は赤、同じ所は基準を暗くした灰色
			unsigned char g = static_cast<unsigned char>((a[i * 4] + a[i * 4 + 1] + a[i * 4 + 2]) / 3 / 3);
			diff[i * 4 + 0] = same ? g : 255;
			diff[i * 4 + 1] = same ? g : 0;
			diff[i * 4 + 2] = same ? g : 0;
			diff[i * 4 + 3] = 255;
		}
		if (result > 0 && !diffPath.empty())
		{
			stbi_flip_vertically_on_write(0);
			stbi_write_png(diffPath.c_str(), w1, h1, 4, diff.data(), w1 * 4);
		}
	}
	stbi_image_free(a);
	stbi_image_free(b);
	return result;
}
