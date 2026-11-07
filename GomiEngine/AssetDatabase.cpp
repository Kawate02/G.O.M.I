#include "AssetDatabase.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stb_image.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
	std::filesystem::path executableDir()
	{
#ifdef _WIN32
		wchar_t buf[MAX_PATH];
		DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
		if (n > 0 && n < MAX_PATH) return std::filesystem::path(buf).parent_path();
#endif
		return std::filesystem::current_path();
	}
}

std::filesystem::path AssetDatabase::findRoot(const std::string& requested)
{
	if (!requested.empty()) return std::filesystem::absolute(requested);
	auto besideExe = executableDir() / "assets";
	if (std::filesystem::is_directory(besideExe)) return besideExe;
#ifdef GOMI_DEFAULT_ASSET_DIR
	return std::filesystem::path(GOMI_DEFAULT_ASSET_DIR);
#else
	return std::filesystem::current_path() / "assets";
#endif
}

void AssetDatabase::setRoot(const std::filesystem::path& path)
{
	root = path;
	sizeCache.clear();
	if (!std::filesystem::is_directory(root)) std::cerr << "assets folder not found: " << root.string() << std::endl;
}

std::filesystem::path AssetDatabase::resolve(const AssetID& id) const
{
	return root / std::filesystem::u8path(id);
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
