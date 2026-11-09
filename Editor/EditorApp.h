#pragma once
#include "GlfwWindow.h"
#include "GlfwInput.h"
#include "GlDrawer.h"
#include "MiniaudioBackend.h"
#include "Renderer.h"
#include "AudioPlayer.h"
#include "SceneEditor.h"
#include "DebugOptions.h"
#include "EditorCamera.h"
#include "Replay.h"

#include <imgui.h>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

/// <summary>
/// G.O.M.I のエディタ。中身の編集は SceneEditor に任せ、ここは ImGui の画面だけを持つ
/// </summary>
/// <summary>
/// std::cerr に書かれた行を受け取って関数に渡す(エンジンの警告をエディタのログに集める)。元の出力先にも流す
/// </summary>
class LineCapture : public std::streambuf
{
private:
	std::streambuf* original;
	std::string line;
	std::function<void(const std::string&)> onLine;
protected:
	int overflow(int c) override
	{
		if (c == EOF) return 0;
		if (original) original->sputc(static_cast<char>(c));
		if (c == '\n')
		{
			if (!line.empty() && onLine) onLine(line);
			line.clear();
		}
		else line.push_back(static_cast<char>(c));
		return c;
	}
public:
	LineCapture(std::streambuf* original, std::function<void(const std::string&)> onLine) : original(original), onLine(std::move(onLine)) {}
};

enum class LogLevel
{
	Info,
	Warning,
	Error,
};

/// <summary>
/// エディタの操作1つ。メニュー・ショートカット・コマンドパレット・起動オプション(--command)が同じ ID で呼ぶ
/// </summary>
struct EditorCommand
{
	std::string id;        //"scene.save" のような名前(起動オプション・将来の AI からの呼び出しに使う)
	std::string label;     //メニュー・パレットに出す名前
	std::string shortcut;  //表示用("Ctrl+S")
	ImGuiKeyChord chord = 0;
	std::function<bool()> enabled;          //空なら常に使える
	std::function<bool()> shortcutWhen;     //ショートカットが効く条件(空なら enabled と同じ)
	std::function<void(const std::string&)> run;
	std::function<std::vector<std::string>()> choices; //引数の候補(あれば引数が要る)
	std::string argName;   //引数の説明(一覧用)
	inline bool isEnabled() const { return !enabled || enabled(); }
};

class EditorApp
{
private:
	DebugOptions debug;
	GlfwWindow window;
	GlfwInput input{ window };
	GlDrawer drawer{ window };
	MiniaudioBackend audioBackend;
	InputState inputState;
	RenderQueue renderQueue;
	AudioQueue audioQueue;
	Renderer renderer;
	AudioPlayer audioPlayer;
	EventRequest<AudioCommand> audioRequest{ Event::Audio_Play };
	std::unique_ptr<SceneEditor> editor;

	int gameTarget = -1;
	int gameWidth = 800;
	int gameHeight = 600;
	int selected = -1; //editor->entities() の何番目か
	bool useEditorCamera = true;
	EditorCamera camera;
	ImVec2 viewMin{ 0, 0 };
	ImVec2 viewSize{ 0, 0 };
	bool viewHovered = false;
	//3D の見た目(前のフレームの値。ギズモとクリック選択に使う)
	bool view3D = false;
	float viewFov = 60;
	float viewNear = 0.1f;
	float viewFar = 1000;
	int gizmoOperation = 0; //0: 移動, 1: 回転, 2: 拡大
	bool gizmoWasUsing = false;
	//2D のスプライトをドラッグで動かす
	bool drag2D = false;
	bool drag2DMoved = false;
	ImVec2 dragMouseStart{ 0, 0 };
	Vec3 dragPosStart;
	/// <summary>
	/// スプライトの四角(ゲームの画面の座標)。スプライトでなければ false
	/// </summary>
	bool spriteRect(Entity e, Vec2& min, Vec2& max);
	void drawSelection2D();

	struct LogLine
	{
		LogLevel level;
		std::string text;
	};
	std::vector<LogLine> logLines;
	std::string lastProblem;     //状態バーに出す最後の警告・エラー
	LogLevel lastProblemLevel = LogLevel::Info;
	bool problemSeen = true;     //ログを開いたら色を戻す
	std::unique_ptr<LineCapture> cerrCapture;
	std::streambuf* originalCerr = nullptr;
	bool fixedStep = true;       //再生を 1/60 秒固定で進める(毎回同じ結果にする)
	bool focusLog = false;
	bool inspectorPlaying = false; //再生の切り替わりで「実行時の値」を開け閉めする
	std::vector<std::string> assetFiles;
	double assetsListedAt = -100;

	// ---- 再現テスト(操作の記録と再生)
	struct ReplayRun
	{
		std::string name;
		Replay replay;
		std::optional<ReplayPlayer> player;
		std::optional<ReplayCheck> check;
		int seekTo = 0;        //ここまで速く進めて一時停止する
		bool fast = false;     //1フレームに何コマも進める
		bool finished = false;
		int lastChecked = 0;   //比べ済みの確認点
		//巻き戻し用に覚えた状態(SNAPSHOT_EVERY フレームごと)
		struct Snapshot
		{
			Json state;          //SceneEditor::playState()
			std::string random;
			ReplayPlayer player;
			ReplayCheck check;
			int lastChecked = 0;
		};
		std::map<int, Snapshot> snapshots;
		bool restored = false; //覚えた状態から流し直した(結果が違えば、最初から流して確かめるよう出す)
	};
	struct ReplaySummary
	{
		bool passed = false;
		bool update = false;   //基準を作り直した
		std::vector<std::string> lines;
	};
	std::optional<ReplayRun> replayRun;
	std::optional<ReplayRecorder> recorder;
	int recordLastFrame = 0;
	std::vector<std::string> replayQueue;   //「全部検査」の残り
	std::map<std::string, ReplaySummary> replaySummaries;
	std::map<std::string, Replay> replayFiles;
	std::string replaySelected;
	double replaysListedAt = -100;
	bool openRecordSave = false;
	Replay pendingRecord;
	std::string recordName;
	bool recordThenUpdate = true;

	// ---- 保存済みとの差・外で変わったファイル
	std::vector<SceneChange> changes;      //保存済み → 今
	double changesAt = -100;
	std::vector<SceneChange> fileChangeList; //外で入った変更(保存済み → ファイル)
	bool openFileConflict = false;
	bool focusChanges = false;

	// ---- コマンド
	std::vector<EditorCommand> commands;
	std::vector<std::string> recentCommands; //"ID\n引数"。新しい順
	bool openPalette = false;
	std::string paletteQuery;
	int paletteCursor = 0;
	bool resetLayout = false;

	// ---- ヒエラルキー
	std::string hierarchyFilter;
	std::string enumFilter;     //選択欄の中の絞り込み
	bool focusFilter = false;
	int renaming = -1;          //名前を直している Entity の並び順
	bool renameFocus = false;
	std::string renameText;
	void startRename(int index);

	// ---- プレハブ
	std::string returnScene;    //プレハブを開く前のシーン(「シーンに戻る」の行き先)
	bool openPrefabSave = false;
	std::string prefabSavePath;
	void editPrefab(const std::string& prefab);
	void savePrefab(const std::string& prefab);

	//保存していない変更がある時の確認
	enum class Pending { None, NewScene, OpenScene, Quit };
	Pending pending = Pending::None;
	std::string pendingScene;
	bool openSaveAs = false;
	std::string saveAsPath;
	bool quit = false;
	bool layoutBuilt = false;

	void log(const std::string& message, LogLevel level = LogLevel::Info);
	void refreshAssets(bool force = false);
	std::vector<std::string> assetsIn(const std::string& folder, std::initializer_list<const char*> exts);
	Entity selectedEntity();

	void request(Pending action, const std::string& scene = "");
	void doPending();
	void saveScene();

	void drawMenuBar();
	void drawToolbar();
	void drawStatusBar();
	void togglePlay();
	void drawHierarchy();
	void drawInspector();
	bool drawField(Entity e, const struct ComponentInfo& info, const struct FieldInfo& f);
	void drawFieldRows(Entity e, const struct ComponentInfo& info, bool runtime);
	void drawSceneView();
	void drawAssets();
	void drawSystems();
	void drawLog();
	void drawPopups();
	void buildDefaultLayout(ImGuiID dockspace);
	void handleShortcuts();
	void updateCamera();
	void drawGizmo();
	void pickAt(const ImVec2& mouse);
	glm::mat4 viewProjection() const;
	void applyScriptedInput(int frame);

	std::filesystem::path replayDir() const;
	void refreshReplays(bool force = false);
	bool canStartReplay(std::string* reason = nullptr) const;
	void startRecording();
	void finishRecording();
	void startReplay(const std::string& name, ReplayCheck::Mode mode, bool fast);
	void endReplay();
	void seekReplay(int frame);
	int replaySteps() const;
	void afterReplayFrame();
	void drawReplays();
	void drawTimeStrip();
	void registerCommands();
	const EditorCommand* findCommand(const std::string& id) const;
	/// <summary>
	/// ID で実行する。知らない・今は使えない・引数が足りない時はログに出して false
	/// </summary>
	bool runCommand(const std::string& id, const std::string& arg = "");
	void menuItem(const std::string& id);
	void drawPalette();
	void printCommands() const;
	void checkAllReplays();
	void checkFiles();
	void reloadScene();
	void refreshChanges(bool force = false);
	void drawChanges();
	void drawChangeTable(const char* id, const std::vector<SceneChange>& list, const char* beforeLabel, const char* afterLabel, bool canSelect);
public:
	explicit EditorApp(const DebugOptions& options);
	~EditorApp();
	int run();
};
