#include "Project.h"
#include "WorldManager.h"
#include "SceneLoader.h"

#include <iostream>

std::unique_ptr<World> WorldManager::createWorld(const std::string& scene)
{
	AssetID path = SceneLoader::scenePath(scene);
	Json data = AssetDatabase::getInstance().readJson(path);
	auto world = std::make_unique<World>(inputState, renderQueue, audioQueue);
	world->setPauseUI(project.value("pauseUI", std::string()));
	world->initialize(data, path);
	return world;
}

void WorldManager::initialize(const std::string& startScene)
{
	auto& db = AssetDatabase::getInstance();
	project = Project::getInstance().read();
	std::string first = startScene.empty() ? project.value("startScene", std::string()) : startScene;
	if (first.empty()) throw std::runtime_error("no start scene: set \"startScene\" in project.json");
	currentWorld = createWorld(first);
	currentScene = first;
}

void WorldManager::changeWorld(const std::string& scene)
{
	std::unique_ptr<World> next;
	try
	{
		next = createWorld(scene);
	}
	catch (const std::exception& e)
	{
		std::cerr << "cannot change world: " << e.what() << std::endl;
		return;
	}
	currentWorld->release();
	currentWorld = std::move(next);
	currentScene = scene;
}

void WorldManager::update(float deltaTime)
{
	if (std::string name; worldChangeRequest.trigger(name))
	{
		changeWorld(name);
	}
	currentWorld->update(deltaTime);
}
