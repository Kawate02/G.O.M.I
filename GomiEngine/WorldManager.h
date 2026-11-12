#pragma once
#include "World.h"
#include "EventRequest.h"

/// <summary>
/// 今の World を1つ持ち、WorldChange イベントでシーンファイルから作り直す
/// </summary>
class WorldManager
{
private:
	EventRequest<std::string> worldChangeRequest{Event::World_Change};
	std::unique_ptr<World> currentWorld;
	std::string currentScene;
	Json project; // プロジェクトの project.json

	InputState& inputState;
	RenderQueue& renderQueue;
	AudioQueue& audioQueue;

	std::unique_ptr<World> createWorld(const std::string& scene);
public:
	WorldManager(InputState& input, RenderQueue& rend, AudioQueue& audio) : inputState(input), renderQueue(rend), audioQueue(audio) {}
	/// <summary>
	/// startScene から始める(空なら project.json の startScene)。読めなければ例外
	/// </summary>
	void initialize(const std::string& startScene = "");
	/// <summary>
	/// 別のシーンに切り替える。読めなければエラーを出して今の World のまま
	/// </summary>
	void changeWorld(const std::string& scene);
	void update(float deltaTime);
	inline World& getWorld() { return *currentWorld; }
	inline const std::string& getCurrentScene() const { return currentScene; }
};
