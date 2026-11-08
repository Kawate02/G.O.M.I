#pragma once
#include "World.h"
#include "SceneLoader.h"
#include "EventRequest.h"

#include <memory>
#include <string>
#include <vector>

/// <summary>
/// エディタの中身(画面を持たない)。シーンを開いて編集・保存し、元に戻す/やり直す、再生して止めたら元に戻す。
/// 編集は「変える前に recordUndo()、変えたら setField() 等」の順で呼ぶ。
/// 元に戻すはシーン全体の JSON を丸ごと積む方式(小さなシーン向け。大きくなったら差分にする)
/// </summary>
class SceneEditor
{
private:
	InputState& inputState;
	RenderQueue& renderQueue;
	AudioQueue& audioQueue;
	std::unique_ptr<World> currentWorld;
	Json project;
	AssetID path;          //空なら未保存の新しいシーン
	bool dirty = false;
	bool playing = false;
	bool paused = false;
	bool stepRequested = false;
	int playFrame = 0;     //再生してから進めたフレーム数
	Json playSnapshot;     //再生前の状態
	std::vector<Json> undoStack;
	std::vector<Json> redoStack;
	EventRequest<std::string> worldChange{ Event::World_Change };
	EventRequest<int> quitRequest{ Event::Game_Quit };

	void rebuild(const Json& scene);
	Json snapshot();
public:
	static constexpr size_t MAX_UNDO = 100;

	SceneEditor(InputState& input, RenderQueue& render, AudioQueue& audio);

	// ---- ファイル
	void newScene();
	/// <summary>
	/// "Field" や "scenes/Field.json"、絶対パスで開く。失敗したら今のシーンのまま false
	/// </summary>
	bool open(const std::string& scene, std::string& error);
	bool save(std::string& error);
	bool saveAs(const AssetID& id, std::string& error);
	inline const AssetID& getPath() const { return path; }
	inline bool isDirty() const { return dirty; }
	inline World& world() { return *currentWorld; }

	// ---- Entity
	/// <summary>
	/// 生きている Entity を並び順で
	/// </summary>
	std::vector<Entity> entities();
	std::string displayName(Entity e);
	Entity createEntity(const std::string& name);
	std::vector<Entity> createFromPrefab(const AssetID& prefab);
	Entity duplicate(Entity e);
	void destroy(Entity e);

	// ---- コンポーネント
	bool addComponent(Entity e, const std::string& component);
	void removeComponent(Entity e, const std::string& component);
	/// <summary>
	/// 1項目を JSON の値で書き換える(元に戻す記録はしない。先に recordUndo を呼ぶ)
	/// </summary>
	bool setField(Entity e, const std::string& component, const std::string& field, const Json& value, std::string* error = nullptr);
	Json getField(Entity e, const std::string& component, const std::string& field);

	// ---- System の並び
	std::vector<std::string> systems();
	void setSystems(const std::vector<std::string>& names);

	// ---- 元に戻す
	/// <summary>
	/// 今の状態を元に戻す先として積む(ドラッグ等の続けての変更は、始めに1回だけ呼ぶ)
	/// </summary>
	void recordUndo();
	inline bool canUndo() const { return !undoStack.empty() && !playing; }
	inline bool canRedo() const { return !redoStack.empty() && !playing; }
	void undo();
	void redo();

	// ---- 再生
	void play();
	void stop();
	inline bool isPlaying() const { return playing; }
	/// <summary>
	/// 再生中に一時停止する/再開する(止めている時は何もしない)
	/// </summary>
	void setPaused(bool value);
	inline bool isPaused() const { return playing && paused; }
	/// <summary>
	/// 一時停止中に1フレームだけ進める(次の update で進む)
	/// </summary>
	void stepFrame();
	inline int getPlayFrame() const { return playFrame; }
	/// <summary>
	/// 再生中はゲームとして進め、止めている時・一時停止中は描画の System だけ動かす
	/// </summary>
	void update(float deltaTime);
};
