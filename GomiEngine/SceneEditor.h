#pragma once
#include "World.h"
#include <functional>
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
	ResourceStore resources; //World より先に置く(World が参照する)
	std::unique_ptr<World> currentWorld;
	Json project;
	AssetID path;          //空なら未保存の新しいシーン
	bool dirty = false;
	bool playing = false;
	bool paused = false;
	bool stepRequested = false;
	int playFrame = 0;     //再生してから進めたフレーム数
	Json playSnapshot;     //再生前の状態
	Json playResources;    //再生前の Resource の値(止めたら戻す)
	bool resourcesDirty = false; //Resource の初期値に保存していない変更がある
	std::string resourceError;   //resources.json を読めなかった理由(読めたら空)
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

	/// <summary>
	/// 止めたまま動かしているアニメーションのプレビュー。World の SpriteAnimation と Renderer を直接書き換えるので、
	/// 書き換える前の値を持っておき、シーンを書き出す時・止める時に戻す
	/// </summary>
	struct AnimationPreview
	{
		bool active = false;
		bool running = false; //時間が進んでいる(false ならコマを止めている)
		Entity entity;
		std::string clip;
		float time = 0;
		bool playing = true;
		std::string path;
		std::vector<float> uv;
	};
	AnimationPreview preview;
	/// <summary>プレビューで書き換えた値と元の値を入れ替える(書き出しの前後で2回呼ぶ)</summary>
	void swapPreviewValues();
	/// <summary>プレビューの今の time のコマを Renderer に書く</summary>
	void applyPreviewFrame();
	struct PreviewHold; //書き出す間だけ元の値に戻す

	void rebuild(const Json& scene);
	/// <summary>元に戻すの1件(シーンと、その時の Resource の値)</summary>
	Json undoEntry(const Json& scene);
	void applyUndoEntry(const Json& entry);
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
	inline ResourceStore& getResources() { return resources; }

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
	/// <summary>
	/// Entity をシーンの entities の1項目の形で返す。1体だけのプレハブの配置なら { "prefab", "overrides" }、それ以外は既定値を省いた { "components" }
	/// </summary>
	Json copyEntity(Entity e);
	/// <summary>
	/// シーンの entities の項目(1つか配列)を足す。シーンを読む時と同じに厳しく調べ、間違いがあれば何もせず false。足した数を count に
	/// </summary>
	bool pasteEntities(const Json& items, int& count, std::string& error);
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

	// ---- Resource(シーンをまたいで残るデータ)。止めている時は初期値(assets/resources.json)、再生中は今の値
	/// <summary>
	/// resources.json を読み直す(保存していない変更は捨てる)。読めなければ理由を返し、値は今のまま
	/// </summary>
	bool loadResources(std::string& error);
	/// <summary>
	/// 今の値を resources.json に書く(止めている時だけ)
	/// </summary>
	bool saveResources(std::string& error);
	inline bool isResourcesDirty() const { return resourcesDirty; }
	/// <summary>resources.json を読めなかった理由(読めたら空)</summary>
	inline const std::string& getResourceError() const { return resourceError; }
	/// <summary>登録されていない名前・項目なら null</summary>
	Json getResourceField(const std::string& resource, const std::string& field);
	/// <summary>
	/// 1項目を JSON の値で書き換える(元に戻す記録はしない。止めている時は先に recordResourceUndo を呼ぶ)
	/// </summary>
	bool setResourceField(const std::string& resource, const std::string& field, const Json& value, std::string* error = nullptr);
	/// <summary>
	/// Resource の初期値を変える前に呼ぶ(元に戻す先として積む。再生中は何もしない)
	/// </summary>
	void recordResourceUndo();

	// ---- アニメーションのプレビュー(止めている時だけ。選んだ Entity の SpriteAnimation を、シーンを変えずに動かす)
	/// <summary>
	/// e の今のクリップの再生を始める(別の Entity のプレビュー中なら先に止める)。クリップを読めない・SpriteAnimation か Renderer が無い・再生中なら false
	/// </summary>
	bool startAnimationPreview(Entity e, std::string& error);
	/// <summary>
	/// プレビューを止めて、Entity の SpriteAnimation と Renderer を始める前の値に戻す
	/// </summary>
	void stopAnimationPreview();
	/// <summary>
	/// プレビュー中に時間を止める/再開する
	/// </summary>
	void setAnimationPreviewRunning(bool running);
	/// <summary>
	/// e のクリップを delta コマ進める(負なら戻す)。時間を止める。プレビューが始まっていなければ始める。ループするクリップは端でつながり、しないクリップは端で止まる
	/// </summary>
	bool stepAnimationPreview(Entity e, int delta, std::string& error);
	inline bool isAnimationPreviewing() const { return preview.active; }
	inline bool isAnimationPreviewRunning() const { return preview.active && preview.running; }
	inline Entity previewEntity() const { return preview.entity; }
	/// <summary>
	/// プレビュー中の今のコマの番号(プレビューしていなければ -1)
	/// </summary>
	int animationPreviewFrame();
	/// <summary>
	/// クリップの JSON を assets/animations/ などに作る(シート: columns・rows・count(0 で全マス)・fps)。画像が読めない・列や行が 0 以下なら false
	/// </summary>
	bool createSheetClip(const AssetID& id, const AssetID& sheet, int columns, int rows, int count, float fps, std::string& error);
	/// <summary>
	/// 連番の画像からクリップの JSON を作る(images/walk_ の後に 00, 01, … と続く .png を拾う。1枚も無ければ false)
	/// </summary>
	bool createSequenceClip(const AssetID& id, const std::string& imagePrefix, float fps, std::string& error);

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
	/// <summary>
	/// ゲームのコード(DLL)を差し替える。今の World を JSON にして消し → swap → 同じ JSON で作り直す。
	/// swap が失敗するか新しいコードでシーンが読めなければ、revert で前のコードに戻して作り直し、false と理由を返す。止めている時だけ
	/// </summary>
	bool swapCode(const std::function<bool(std::string& error)>& swap, const std::function<void()>& revert, std::string& error);
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
	/// 再生中の今の状態(World・Resource・シーンの名前・フレーム・ポーズなど)。巻き戻し用
	/// </summary>
	Json playState();
	/// <summary>
	/// 再生中に playState() で覚えた状態に戻す(再生前の状態は変えない)
	/// </summary>
	bool restorePlayState(const Json& state, std::string& error);
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
