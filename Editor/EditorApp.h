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

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

/// <summary>
/// G.O.M.I のエディタ。中身の編集は SceneEditor に任せ、ここは ImGui の画面だけを持つ
/// </summary>
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

	std::vector<std::string> logLines;
	std::vector<std::string> assetFiles;
	double assetsListedAt = -100;

	//保存していない変更がある時の確認
	enum class Pending { None, NewScene, OpenScene, Quit };
	Pending pending = Pending::None;
	std::string pendingScene;
	bool openSaveAs = false;
	std::string saveAsPath;
	bool quit = false;
	bool layoutBuilt = false;

	void log(const std::string& message);
	void refreshAssets(bool force = false);
	std::vector<std::string> assetsIn(const std::string& folder, std::initializer_list<const char*> exts);
	Entity selectedEntity();

	void request(Pending action, const std::string& scene = "");
	void doPending();
	void saveScene();

	void drawMenuBar();
	void drawHierarchy();
	void drawInspector();
	bool drawField(Entity e, const struct ComponentInfo& info, const struct FieldInfo& f);
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
public:
	explicit EditorApp(const DebugOptions& options);
	~EditorApp();
	int run();
};
