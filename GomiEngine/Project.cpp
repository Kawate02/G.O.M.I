#include "Project.h"
#include "AssetDatabase.h"
#include "GameModule.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

Project& Project::getInstance()
{
	static Project instance;
	return instance;
}

fs::path Project::findRoot(const std::string& requested)
{
	if (!requested.empty()) return fs::absolute(fs::path(std::u8string(requested.begin(), requested.end()))); //起動オプションは UTF-8(DebugOptions が読み直す)
	std::error_code ec;
	auto besideExe = GameModule::exeDir();
	if (fs::is_regular_file(besideExe / FILE_NAME, ec)) return besideExe;
#ifdef GOMI_DEFAULT_PROJECT_DIR
	const std::string dir = GOMI_DEFAULT_PROJECT_DIR; //UTF-8(/utf-8 で作る)
	return fs::path(std::u8string(dir.begin(), dir.end()));
#else
	return fs::current_path();
#endif
}

void Project::open(const fs::path& folder)
{
	root = folder;
	std::error_code ec;
	if (!fs::is_regular_file(file(), ec)) std::cerr << "project.json not found: " << file().string() << std::endl;
	AssetDatabase::getInstance().setRoot(assetsDir());
}

Json Project::read() const
{
	std::error_code ec;
	if (!fs::is_regular_file(file(), ec)) return Json::object();
	std::ifstream in(file());
	try
	{
		return Json::parse(in);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(std::string(FILE_NAME) + ": " + e.what());
	}
}

std::string Project::engineVersion() const
{
	//壊れた project.json は startScene を読む所で知らせるので、ここでは無いのと同じに扱う
	Json json;
	try { json = read(); }
	catch (const std::exception&) { return {}; }
	auto it = json.find("engine");
	return it != json.end() && it->is_string() ? it->get<std::string>() : std::string();
}
