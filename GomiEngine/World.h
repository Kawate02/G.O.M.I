#pragma once
#include "Utilities.h"
#include "SystemInclude.h"
#include "ComponentManager.h"
#include "UIManager.h"
#include "WorldEventBus.h"
#include "Camera.h"
#include "JsonConvert.h"
#include "AssetDatabase.h"
#include "CollisionContext.h"

#include "InputState.h"
#include "RenderQueue.h"
#include "AudioQueue.h"
#include "EventRequest.h"

/// <summary>
/// その Entity がどのプレハブの何番目から作られたか。instance が同じものは同じ配置から作られた仲間
/// </summary>
struct PrefabLink
{
	AssetID prefab;
	int index = 0;
	int instance = 0;
};

class World
{
protected:
	Camera camera;
	ComponentManager component;
	CollisionContext collision;
	UIManager uiManager;
	std::vector<std::unique_ptr<ISystem>> systems;
	std::vector<std::string> systemNames;
	Json uiEntries = Json::array(); //シーンの "ui" をそのまま覚えて保存に使う
	AssetID pauseUIPath;
	InputState& inputState;
	RenderQueue& renderQueue;
	AudioQueue& audioQueue;
	WorldEventBus eventBus;

	std::vector<Entity> entities;
	std::vector<uint32_t> generations;
	std::queue<uint32_t> freeIDs;
	std::vector<Entity> pendingDestroy; //フレームの最後にコンポーネントを消す Entity
	void flushDestroyed();
	std::unordered_map<uint32_t, std::pair<Entity, PrefabLink>> prefabLinks;
	int nextPrefabInstance = 1;

	EventRequest<int> pauseClose{ Event::Pause_Close };
	UIHandle pauseUI = { 0, 0 };
	bool pause = false;

	int frameCount = 0;
	bool simulating = true;
	float deltaTime = 0.0f;
public:
	World(InputState& input, RenderQueue& rend, AudioQueue& aud) : inputState(input), renderQueue(rend), audioQueue(aud) {}
	/// <summary>
	/// シーンの JSON から System・Entity・UI を作る。source はエラー表示用のファイル名
	/// </summary>
	void initialize(const Json& scene, const std::string& source);
	/// <summary>
	/// simulate が false なら、エディタ用に描画の System(runsInEditMode)だけ動かし、入力・ポーズ・UI の反応は止める
	/// </summary>
	void update(float dt, bool simulate = true);
	void release();
	Entity createEntity();
	/// <summary>
	/// すぐに alive が false になる。コンポーネントの削除と ID の再利用はフレームの最後(each の外)で行う
	/// </summary>
	void destroyEntity(Entity e);
	/// <summary>
	/// 名前で登録済みの System を末尾に足す(知らない名前なら例外)
	/// </summary>
	void addSystem(const std::string& name);
	UIHandle spawnUI(const AssetID& prefab, Vec2 pos, int layer);
	inline void setPauseUI(const AssetID& path) { pauseUIPath = path; }
	/// <summary>
	/// シーンの JSON に入らない、動いている間の状態(フレーム数・ポーズ中か)。巻き戻しで覚えて戻す
	/// </summary>
	Json runState() const;
	void restoreRunState(const Json& state);
	inline const std::vector<std::string>& getSystemNames() const { return systemNames; }
	inline const Json& getUIEntries() const { return uiEntries; }
	inline void setUIEntries(const Json& entries) { uiEntries = entries; }
	inline int newPrefabInstance() { return nextPrefabInstance++; }
	inline void setPrefabLink(Entity e, const PrefabLink& link) { prefabLinks[e.id] = { e, link }; }
	inline void clearPrefabLink(Entity e) { prefabLinks.erase(e.id); }
	/// <summary>
	/// プレハブから作られていなければ nullptr
	/// </summary>
	inline const PrefabLink* getPrefabLink(Entity e) const
	{
		auto it = prefabLinks.find(e.id);
		return it != prefabLinks.end() && it->second.first == e ? &it->second.second : nullptr;
	}
	inline ComponentManager& getComponentManager() { return component; }
	inline Camera& getCamera() { return camera; }
	/// <summary>
	/// ゲームとして進めているか(エディタで止めている時は false)。System がデータを書き換えるかの判断に使う
	/// </summary>
	inline bool isSimulating() const { return simulating; }
	inline bool alive(Entity e) const { return e.id < generations.size() && e.generation == generations[e.id]; }
	inline WorldEventBus& event() { return eventBus; }
	inline CollisionContext& col() { return collision; }
	/// <summary>生きている Entity だけを ID 順に詰めて返す(毎回コピー。毎フレームの処理では ComponentManager::each を使う)</summary>
	std::vector<Entity> getEntities() const;
	inline InputState& getInputState() { return inputState; }
	inline RenderQueue& getRenderQueue() { return renderQueue; }
	inline AudioQueue& getAudioQueue() { return audioQueue; }
	inline UIManager& getUIManager() { return uiManager; }
};