#pragma once
#include "World.h"
#include "SceneLoader.h"
#include "EventRequest.h"
#include "SceneDiff.h"

#include <filesystem>
#include <map>

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
	std::string playScene; //再生中の今のシーンの名前(ゲームの中で切り替わると変わる)
	std::vector<Json> undoStack;
	std::vector<Json> redoStack;
	std::unordered_map<AssetID, std::vector<Json>> prefabCache; //プレハブの各 Entity のコンポーネント(既定値で埋めたもの)
	EventRequest<std::string> worldChange{ Event::World_Change };
	EventRequest<int> quitRequest{ Event::Game_Quit };
	Json savedScene;       //開いた・保存した時のファイルの中身(差分の元)
	struct FileStamp
	{
		std::filesystem::file_time_type time;
		uintmax_t size = 0;
		bool operator==(const FileStamp& o) const { return time == o.time && size == o.size; }
	};
	std::map<AssetID, FileStamp> fileStamps; //assets の JSON の前回の更新時刻
	bool filesWatched = false;
	bool scenePending = false; //開いているシーンが外で変わったが、未保存の変更があるので待っている
	Json diskScene;            //その時のファイルの中身

	void rebuild(const Json& scene);
	Json snapshot();
	const std::vector<Json>& prefabDefs(const AssetID& prefab);
	std::map<AssetID, FileStamp> scanFiles() const;
	void rememberStamp(const AssetID& id);
	/// <summary>
	/// ファイルを読んでシーンの形にする(プレハブなら System を足して包む)
	/// </summary>
	Json readAsScene(const AssetID& id);
	/// <summary>
	/// id に書く中身(プレハブなら Entity の並びだけを短く)
	/// </summary>
	Json fileContent(const AssetID& id);
	Json wrapPrefab(const Json& prefab) const;
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
	/// <summary>
	/// prefabs/ の下のファイルはプレハブとして開く・保存する({ "entities": [...] } だけのファイル)
	/// </summary>
	static bool isPrefabPath(const AssetID& id);
	/// <summary>
	/// プレハブを開いて直している(再生はできない)
	/// </summary>
	inline bool isPrefabMode() const { return isPrefabPath(path); }
	/// <summary>
	/// 選んだ Entity をプレハブのファイルに書き、その Entity をそのプレハブの配置にする(元に戻す記録もする)
	/// </summary>
	bool saveAsPrefab(Entity e, const AssetID& id, std::string& error);
	inline bool isDirty() const { return dirty; }
	inline World& world() { return *currentWorld; }

	// ---- Entity
	/// <summary>
	/// 生きている Entity を並び順で
	/// </summary>
	std::vector<Entity> entities();
	/// <summary>
	/// ヒエラルキーに出す呼び名。同じ呼び名は「crate #2」のように番号を付ける(差分の呼び名と同じ)
	/// </summary>
	std::string displayName(Entity e);
	/// <summary>
	/// entities() の順の呼び名
	/// </summary>
	std::vector<std::string> displayNames();
	/// <summary>
	/// 名前を変える(Name が無ければ付ける。元に戻す記録もする)
	/// </summary>
	void rename(Entity e, const std::string& name);
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
	/// <summary>
	/// プレハブから置いた Entity なら、その項目のプレハブ側の値。プレハブから置いていない・プレハブに無いコンポーネントなら null
	/// </summary>
	Json prefabField(Entity e, const std::string& component, const std::string& field);
	/// <summary>
	/// 項目をプレハブの値に戻す(元に戻す記録もする)。戻せなければ false
	/// </summary>
	bool revertToPrefab(Entity e, const std::string& component, const std::string& field);
	/// <summary>
	/// プレハブを読み直す(外でプレハブが変わった時)
	/// </summary>
	inline void clearPrefabCache() { prefabCache.clear(); }

	// ---- 保存済みとの差・外で変わったファイル
	/// <summary>
	/// 保存済み(開いた・保存した時のファイル)から今の状態への違い。再生中は再生前の状態と比べる
	/// </summary>
	std::vector<SceneChange> changesFromSaved();
	struct FileCheck
	{
		std::vector<AssetID> changed; //変わった・増えた・消えた JSON
		bool reloaded = false;        //開いているシーンを読み直した(未保存の変更が無かった)
		bool sceneConflict = false;   //開いているシーンが変わったが、未保存の変更があるので待っている
		bool refreshed = false;       //プレハブ・UI 等を読み直した(自分の変更は残る)
		std::string error;            //読めなかった(書きかけ・壊れている)。今の状態のまま
	};
	/// <summary>
	/// assets の JSON が外で変わったか調べ、変わっていれば読み直す。最初の呼び出しは覚えるだけ。再生中は何もしない
	/// </summary>
	FileCheck checkFiles();
	inline bool hasSceneConflict() const { return scenePending; }
	/// <summary>
	/// 外でファイルに入った変更(保存済み → 今のファイル)
	/// </summary>
	std::vector<SceneChange> fileChanges();
	/// <summary>
	/// 未保存の変更を捨ててファイルを読み直す(元に戻すで今の状態に戻れる)
	/// </summary>
	bool reloadFromFile(std::string& error);
	/// <summary>
	/// 外の変更を読まずに今の状態を続ける(次の保存でファイルを上書きする)
	/// </summary>
	void keepEdits();

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
	/// 次の update でゲームが1フレーム進むか(記録・再生の入力をその時だけ渡す)
	/// </summary>
	inline bool willAdvance() const { return playing && (!paused || stepRequested); }
	/// <summary>
	/// 再生中の今のシーンの名前(scenes/Title.json なら Title)。ゲームの WorldManager と同じ呼び方
	/// </summary>
	inline const std::string& getPlayScene() const { return playScene; }
	/// <summary>
	/// シーンのファイルの場所から、ゲームでの呼び名にする(scenes/X.json → X)
	/// </summary>
	static std::string sceneName(const std::string& path);
	/// <summary>
	/// 再生中はゲームとして進め、止めている時・一時停止中は描画の System だけ動かす
	/// </summary>
	void update(float deltaTime);
};
