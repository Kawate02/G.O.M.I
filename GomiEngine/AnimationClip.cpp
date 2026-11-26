#include "AnimationClip.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>

float AnimationClip::duration() const
{
	float units = 0;
	for (float d : durations) units += d;
	return units / fps;
}

int AnimationClip::frameAt(float time) const
{
	if (frames.empty()) return 0;
	float total = 0;
	for (float d : durations) total += d;
	float units = std::max(0.0f, time) * fps;
	if (loop) units = std::fmod(units, total);
	else if (units >= total) return static_cast<int>(frames.size()) - 1;
	float end = 0;
	for (size_t i = 0; i < durations.size(); i++)
	{
		end += durations[i];
		if (units < end) return static_cast<int>(i);
	}
	return static_cast<int>(frames.size()) - 1;
}

bool AnimationClip::finishedAt(float time) const
{
	return !loop && time >= duration();
}

static int readInt(const Json& json, const char* key, const std::string& source)
{
	if (!json.contains(key)) throw std::runtime_error(source + ": \"" + key + "\" is required");
	if (!json[key].is_number_integer()) throw std::runtime_error(source + ": \"" + key + "\" must be an integer");
	return json[key].get<int>();
}

AnimationClip AnimationClip::parse(const Json& json, const std::string& source)
{
	if (!json.is_object()) throw std::runtime_error(source + ": expected an object");
	static const std::set<std::string> known = { "sheet", "columns", "rows", "count", "frames", "fps", "loop", "durations" };
	for (auto& [key, v] : json.items())
	{
		if (!known.count(key)) throw std::runtime_error(source + ": unknown key \"" + key + "\"");
	}

	AnimationClip clip;
	if (json.contains("fps"))
	{
		if (!json["fps"].is_number() || json["fps"].get<float>() <= 0) throw std::runtime_error(source + ": \"fps\" must be a number greater than 0");
		clip.fps = json["fps"].get<float>();
	}
	if (json.contains("loop"))
	{
		if (!json["loop"].is_boolean()) throw std::runtime_error(source + ": \"loop\" must be true or false");
		clip.loop = json["loop"].get<bool>();
	}

	auto& db = AssetDatabase::getInstance();
	bool hasSheet = json.contains("sheet"), hasFrames = json.contains("frames");
	if (hasSheet == hasFrames) throw std::runtime_error(source + ": write either \"sheet\" or \"frames\"");
	if (hasSheet)
	{
		if (!json["sheet"].is_string()) throw std::runtime_error(source + ": \"sheet\" must be a string");
		AssetID sheet = json["sheet"].get<std::string>();
		if (!db.exists(sheet)) throw std::runtime_error(source + ": image not found: " + sheet);
		int columns = readInt(json, "columns", source), rows = readInt(json, "rows", source);
		if (columns < 1 || rows < 1) throw std::runtime_error(source + ": \"columns\" and \"rows\" must be 1 or more");
		int count = columns * rows;
		if (json.contains("count"))
		{
			count = readInt(json, "count", source);
			if (count < 1 || count > columns * rows) throw std::runtime_error(source + ": \"count\" must be between 1 and columns x rows");
		}
		for (int i = 0; i < count; i++)
		{
			int col = i % columns, row = i / columns;
			AnimationFrame f;
			f.image = sheet;
			f.uv = { static_cast<float>(col) / columns, static_cast<float>(row) / rows, static_cast<float>(col + 1) / columns, static_cast<float>(row + 1) / rows };
			clip.frames.push_back(f);
		}
	}
	else
	{
		for (const char* key : { "columns", "rows", "count" })
		{
			if (json.contains(key)) throw std::runtime_error(source + ": \"" + key + "\" is only for \"sheet\"");
		}
		const Json& list = json["frames"];
		if (!list.is_array() || list.empty()) throw std::runtime_error(source + ": \"frames\" must be a non-empty array of images");
		Vec2 size;
		for (size_t i = 0; i < list.size(); i++)
		{
			if (!list[i].is_string()) throw std::runtime_error(source + ": \"frames\" must be strings");
			AssetID image = list[i].get<std::string>();
			if (!db.exists(image)) throw std::runtime_error(source + ": image not found: " + image);
			Vec2 s = db.imageSize(image);
			if (s.x <= 0 || s.y <= 0) throw std::runtime_error(source + ": cannot read image: " + image);
			if (i == 0) size = s;
			else if (s.x != size.x || s.y != size.y) throw std::runtime_error(source + ": image size differs from the first frame: " + image);
			AnimationFrame f;
			f.image = image;
			clip.frames.push_back(f);
		}
	}

	clip.durations.assign(clip.frames.size(), 1.0f);
	if (json.contains("durations"))
	{
		const Json& d = json["durations"];
		if (!d.is_array() || d.size() != clip.frames.size()) throw std::runtime_error(source + ": \"durations\" must have one number per frame");
		for (size_t i = 0; i < d.size(); i++)
		{
			if (!d[i].is_number() || d[i].get<float>() <= 0) throw std::runtime_error(source + ": \"durations\" must be numbers greater than 0");
			clip.durations[i] = d[i].get<float>();
		}
	}
	return clip;
}

namespace
{
	// 読めなかったパスも覚えて(nullptr)、毎フレーム読み直さない。実体を GomiCore に置くので DLL をまたいで1つになる
	std::map<AssetID, std::unique_ptr<AnimationClip>>& cache()
	{
		static std::map<AssetID, std::unique_ptr<AnimationClip>> instance;
		return instance;
	}
}

const AnimationClip* AnimationClip::load(const AssetID& path)
{
	auto it = cache().find(path);
	if (it != cache().end()) return it->second.get();
	std::unique_ptr<AnimationClip> clip;
	try
	{
		clip = std::make_unique<AnimationClip>(parse(AssetDatabase::getInstance().readJson(path), path));
	}
	catch (const std::exception& e)
	{
		AssetDatabase::getInstance().warnOnce("animation:" + path, std::string("animation clip error (") + e.what() + ")");
	}
	return (cache()[path] = std::move(clip)).get();
}

void AnimationClip::clearCache()
{
	cache().clear();
}
