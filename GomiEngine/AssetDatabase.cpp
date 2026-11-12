#include "AssetDatabase.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stb_image.h>

void AssetDatabase::setRoot(const std::filesystem::path& path)
{
	root = path;
	sizeCache.clear();
	if (!std::filesystem::is_directory(root)) std::cerr << "assets folder not found: " << root.string() << std::endl;
}

std::filesystem::path AssetDatabase::resolve(const AssetID& id) const
{
	//ID は UTF-8 なので char8_t として渡す(u8path は C++20 で非推奨)
	return root / std::filesystem::path(std::u8string(id.begin(), id.end()));
}

bool AssetDatabase::exists(const AssetID& id) const
{
	return !id.empty() && std::filesystem::is_regular_file(resolve(id));
}

std::vector<unsigned char> AssetDatabase::readBytes(const AssetID& id) const
{
	if (!exists(id)) throw std::runtime_error("asset not found: " + id);
	std::ifstream file(resolve(id), std::ios::binary);
	return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

Json AssetDatabase::readJson(const AssetID& id) const
{
	if (!exists(id)) throw std::runtime_error("asset not found: " + id);
	std::ifstream file(resolve(id));
	try
	{
		return Json::parse(file);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(id + ": " + e.what());
	}
}

void AssetDatabase::writeJson(const AssetID& id, const Json& json) const
{
	auto path = resolve(id);
	std::filesystem::create_directories(path.parent_path());
	std::ofstream file(path);
	if (!file) throw std::runtime_error("cannot write: " + id);
	file << json.dump(2) << "\n";
}

Vec2 AssetDatabase::imageSize(const AssetID& id)
{
	auto it = sizeCache.find(id);
	if (it != sizeCache.end()) return it->second;
	Vec2 size;
	if (exists(id))
	{
		int w, h, channels;
		auto bytes = readBytes(id);
		if (stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &channels)) size = Vec2(static_cast<float>(w), static_cast<float>(h));
		else warnOnce(id, "not an image");
	}
	else if (!id.empty())
	{
		warnOnce(id, "asset not found");
	}
	sizeCache[id] = size;
	return size;
}

void AssetDatabase::warnOnce(const AssetID& id, const std::string& message)
{
	if (warned.insert(id).second) std::cerr << message << ": " << id << std::endl;
}

AssetDatabase& AssetDatabase::getInstance()
{
	static AssetDatabase instance;
	return instance;
}
