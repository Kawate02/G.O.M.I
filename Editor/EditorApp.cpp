#include "EditorApp.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "SettingManager.h"
#include "ResourceManager.h"
#include "SceneLoader.h"
#include "Random.h"

#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include <ImGuizmo.h>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace
{
	const char* WIN_HIERARCHY = "ヒエラルキー";
	const char* WIN_INSPECTOR = "インスペクタ";
	const char* WIN_SCENE = "シーン";
	const char* WIN_ASSETS = "アセット";
	const char* WIN_SYSTEMS = "System";
	const char* WIN_LOG = "ログ";
	const char* WIN_REPLAYS = "再現テスト";
	const char* WIN_CHANGES = "差分";
	const char* POPUP_CONFLICT = "外でファイルが変わりました";
	const char* ASSET_PAYLOAD = "GOMI_ASSET";

	bool endsWith(const std::string& s, const char* ext)
	{
		std::string e = ext;
		return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0;
	}
}

namespace
{
	const ImVec4 PLAY_TINT(0.55f, 0.32f, 0.08f, 1.0f); //再生中の帯の色
	const ImVec4 PLAY_TEXT(1.0f, 0.72f, 0.35f, 1.0f);
	const ImVec4 COLOR_WARNING(1.0f, 0.8f, 0.3f, 1.0f);
	const ImVec4 COLOR_ERROR(1.0f, 0.42f, 0.38f, 1.0f);
	const ImVec4 COLOR_OK(0.45f, 0.8f, 0.4f, 1.0f);
	const ImVec4 COLOR_RECORD(0.9f, 0.25f, 0.25f, 1.0f);
	const int FAST_STEPS = 30; //速く流す時に1回の表示で進めるコマ数
	const int SNAPSHOT_EVERY = 60; //記録を流す時、巻き戻し用に状態を覚える間隔(フレーム)
}

EditorApp::EditorApp(const DebugOptions& options) : debug(options)
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	auto& settings = SettingManager::getInstance();
	settings.loadSettings();
	gameWidth = settings.getValue<int>("windowWidth");
	gameHeight = settings.getValue<int>("windowHeight");

	window.setResizable(true);
	window.init(1600, 900, "G.O.M.I Editor");
	drawer.init();
	audioBackend.init();
	gameTarget = drawer.createRenderTarget(gameWidth, gameHeight);
	if (gameTarget < 0) throw std::runtime_error("cannot create the scene view render target");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
	//操作を再現している時は前回の配置を読まない(毎回同じ画面にする)
	static std::string iniPath = (std::filesystem::current_path() / "editor_layout.ini").string();
	io.IniFilename = debug.maxFrames > 0 ? nullptr : iniPath.c_str();
	const char* fonts[] = { "C:/Windows/Fonts/meiryo.ttc", "C:/Windows/Fonts/YuGothM.ttc", "C:/Windows/Fonts/msgothic.ttc" };
	for (auto f : fonts)
	{
		if (std::filesystem::exists(f) && io.Fonts->AddFontFromFileTTF(f, 17.0f)) break;
	}
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 3;
	ImGui_ImplGlfw_InitForOpenGL(window.handle(), true);
	ImGui_ImplOpenGL3_Init("#version 450");

	originalCerr = std::cerr.rdbuf();
	cerrCapture = std::make_unique<LineCapture>(originalCerr, [this](const std::string& line) {
		logLines.push_back({ LogLevel::Warning, line });
		lastProblem = line;
		lastProblemLevel = LogLevel::Warning;
		problemSeen = false;
	});
	std::cerr.rdbuf(cerrCapture.get());

	editor = std::make_unique<SceneEditor>(inputState, renderQueue, audioQueue);
	if (!debug.startWorld.empty())
	{
		std::string error;
		if (!editor->open(debug.startWorld, error)) log("開けませんでした: " + error, LogLevel::Error);
	}
	selected = debug.select;
	registerCommands();
	refreshAssets(true);
}

EditorApp::~EditorApp()
{
	editor.reset();
	if (originalCerr) std::cerr.rdbuf(originalCerr);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void EditorApp::log(const std::string& message, LogLevel level)
{
	logLines.push_back({ level, message });
	if (logLines.size() > 500) logLines.erase(logLines.begin());
	if (level != LogLevel::Info)
	{
		lastProblem = message;
		lastProblemLevel = level;
		problemSeen = false;
	}
	std::cout << message << std::endl;
}

void EditorApp::refreshAssets(bool force)
{
	double now = ImGui::GetCurrentContext() ? ImGui::GetTime() : 0.0;
	if (!force && now - assetsListedAt < 2.0) return;
	assetsListedAt = now;
	assetFiles.clear();
	auto root = AssetDatabase::getInstance().getRoot();
	std::error_code ec;
	for (auto it = std::filesystem::recursive_directory_iterator(root, ec); !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
	{
		if (!it->is_regular_file()) continue;
		auto rel = std::filesystem::relative(it->path(), root, ec).generic_u8string();
		assetFiles.push_back(std::string(rel.begin(), rel.end()));
	}
	std::sort(assetFiles.begin(), assetFiles.end());
}

std::vector<std::string> EditorApp::assetsIn(const std::string& folder, std::initializer_list<const char*> exts)
{
	std::vector<std::string> result;
	for (auto& f : assetFiles)
	{
		if (!folder.empty() && f.rfind(folder + "/", 0) != 0) continue;
		bool ok = exts.size() == 0;
		for (auto e : exts) ok = ok || endsWith(f, e);
		if (ok) result.push_back(f);
	}
	return result;
}

Entity EditorApp::selectedEntity()
{
	auto list = editor->entities();
	if (selected < 0 || selected >= static_cast<int>(list.size()))
	{
		selected = -1;
		return Entity{ 0, 0xffffffffu };
	}
	return list[selected];
}

// ---- 保存していない変更の確認

void EditorApp::request(Pending action, const std::string& scene)
{
	pending = action;
	pendingScene = scene;
	if (!editor->isDirty()) doPending();
}

void EditorApp::doPending()
{
	std::string error;
	switch (pending)
	{
	case Pending::NewScene:
		editor->newScene();
		selected = -1;
		log("新しいシーンを作りました");
		break;
	case Pending::OpenScene:
		if (editor->open(pendingScene, error))
		{
			selected = -1;
			log("開きました: " + editor->getPath());
		}
		else log("開けませんでした: " + error, LogLevel::Error);
		break;
	case Pending::Quit:
		quit = true;
		break;
	case Pending::None:
		break;
	}
	pending = Pending::None;
}

void EditorApp::saveScene()
{
	if (editor->getPath().empty())
	{
		openSaveAs = true;
		saveAsPath = "scenes/NewScene.json";
		return;
	}
	std::string error;
	if (editor->save(error)) log("保存しました: " + editor->getPath());
	else log("保存できませんでした: " + error, LogLevel::Error);
}

// ---- 画面

void EditorApp::drawMenuBar()
{
	if (!ImGui::BeginMainMenuBar()) return;
	bool playing = editor->isPlaying();
	if (ImGui::BeginMenu("ファイル"))
	{
		menuItem("scene.new");
		if (ImGui::BeginMenu("開く", !playing))
		{
			for (auto& s : assetsIn("scenes", { ".json" }))
			{
				if (ImGui::MenuItem(s.c_str())) runCommand("scene.open", s);
			}
			ImGui::EndMenu();
		}
		menuItem("scene.save");
		menuItem("scene.saveAs");
		menuItem("scene.reload");
		ImGui::Separator();
		if (ImGui::BeginMenu("プレハブを開いて直す", !playing))
		{
			for (auto& s : assetsIn("prefabs", { ".json" }))
			{
				if (ImGui::MenuItem(s.c_str())) runCommand("prefab.open", s);
			}
			ImGui::EndMenu();
		}
		menuItem("prefab.back");
		ImGui::Separator();
		menuItem("app.quit");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("編集"))
	{
		menuItem("edit.undo");
		menuItem("edit.redo");
		ImGui::Separator();
		menuItem("entity.create");
		menuItem("entity.rename");
		menuItem("prefab.saveSelection");
		menuItem("prefab.edit");
		menuItem("entity.copy");
		menuItem("entity.paste");
		menuItem("entity.duplicate");
		menuItem("entity.delete");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("再生"))
	{
		menuItem("play.toggle");
		menuItem("play.pause");
		menuItem("play.step");
		ImGui::Separator();
		if (ImGui::MenuItem("Δt を 1/60 秒に固定", nullptr, fixedStep)) runCommand("play.fixedStep");
		ImGui::Separator();
		menuItem("replay.record");
		menuItem("replay.checkAll");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("表示"))
	{
		menuItem("palette.open");
		ImGui::Separator();
		for (auto& c : commands)
		{
			if (c.id.rfind("window.", 0) == 0) menuItem(c.id);
		}
		ImGui::Separator();
		menuItem("layout.reset");
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void EditorApp::togglePlay()
{
	if (editor->isPlaying()) editor->stop();
	else if (editor->isPrefabMode()) log("プレハブを直している間は再生できません(シーンに戻ってから)", LogLevel::Warning);
	else editor->play();
}

void EditorApp::drawToolbar()
{
	bool playing = editor->isPlaying();
	float height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2;
	if (playing) ImGui::PushStyleColor(ImGuiCol_WindowBg, PLAY_TINT);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	bool open = ImGui::BeginViewportSideBar("##toolbar", ImGui::GetMainViewport(), ImGuiDir_Up, height, flags);
	if (playing) ImGui::PopStyleColor();
	if (!open)
	{
		ImGui::End();
		return;
	}
	auto tip = [](const char* text) { if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s", text); };

	ImGui::BeginDisabled(!playing && editor->isPrefabMode());
	if (ImGui::Button(playing ? "■ 停止" : "▶ 再生")) togglePlay();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", playing ? "止めると、再生中の変更は消えて再生前に戻る (F5)" : editor->isPrefabMode() ? "プレハブはシーンに戻ってから動かす" : "ゲームとして動かす (F5)");
	ImGui::SameLine();
	ImGui::BeginDisabled(!playing);
	if (ImGui::Button(editor->isPaused() ? "▶ 再開" : "|| 一時停止")) editor->setPaused(!editor->isPaused());
	tip("F6");
	ImGui::SameLine();
	if (ImGui::Button(">| 1コマ")) editor->stepFrame();
	tip("一時停止して1フレームだけ進める (F7)");
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (recorder)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RECORD);
		if (ImGui::Button("● 記録中")) editor->stop();
		ImGui::PopStyleColor();
		tip("押すと記録を終えて保存する(■ 停止 と同じ)");
	}
	else
	{
		std::string reason;
		bool can = canStartReplay(&reason);
		ImGui::BeginDisabled(!can);
		if (ImGui::Button("● 記録")) startRecording();
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip("%s", can ? "保存したシーンを最初から再生し、操作を記録する。止めると再現テストとして保存する" : reason.c_str());
	}
	ImGui::SameLine();
	ImGui::Checkbox("Δt 1/60 固定", &fixedStep);
	tip("再生を毎フレーム 1/60 秒ずつ進める。同じ操作から毎回同じ結果になる");

	ImGui::SameLine(0, 24);
	ImGui::TextDisabled("|");
	ImGui::SameLine(0, 24);
	ImGui::RadioButton("移動", &gizmoOperation, 0);
	tip("W");
	ImGui::SameLine();
	ImGui::RadioButton("回転", &gizmoOperation, 1);
	tip("E");
	ImGui::SameLine();
	ImGui::RadioButton("拡大", &gizmoOperation, 2);
	tip("R");
	ImGui::SameLine(0, 24);
	ImGui::Checkbox("エディタカメラ", &useEditorCamera);
	tip("止めている時の 3D をエディタ用のカメラで見る\n右ドラッグ: 回す / 中ドラッグ: 動かす / ホイール: 寄る / F: 選んだ物へ\n左クリック: 物を選ぶ");

	if (playing)
	{
		ImGui::SameLine(0, 24);
		ImGui::TextColored(PLAY_TEXT, editor->isPaused() ? "一時停止中" : "再生中");
		ImGui::SameLine();
		if (recorder) ImGui::TextUnformatted("操作を記録しています");
		else if (replayRun) ImGui::Text("再現テスト %s を流しています", replayRun->name.c_str());
		else ImGui::TextUnformatted("止めると変更は消えます");
	}

	if (editor->isPrefabMode())
	{
		ImGui::SameLine(0, 24);
		ImGui::TextColored(ImVec4(0.45f, 0.7f, 1.0f, 1.0f), "プレハブを直しています");
		if (!returnScene.empty())
		{
			ImGui::SameLine();
			if (ImGui::SmallButton(("← " + returnScene + " に戻る").c_str())) runCommand("prefab.back");
		}
	}

	//右端にシーンのファイル名
	std::string name = (editor->getPath().empty() ? std::string("(新しいシーン)") : editor->getPath()) + (editor->isDirty() ? " *" : "");
	float w = ImGui::CalcTextSize(name.c_str()).x;
	ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 20, ImGui::GetWindowWidth() - w - 16));
	if (playing) ImGui::TextUnformatted(name.c_str()); //色の付いた帯の上では薄い文字が読めない
	else ImGui::TextDisabled("%s", name.c_str());
	ImGui::End();
}

void EditorApp::drawStatusBar()
{
	bool playing = editor->isPlaying();
	float height = ImGui::GetFrameHeight();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 3));
	if (playing) ImGui::PushStyleColor(ImGuiCol_WindowBg, PLAY_TINT);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	bool open = ImGui::BeginViewportSideBar("##status", ImGui::GetMainViewport(), ImGuiDir_Down, height, flags);
	if (playing) ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	if (!open)
	{
		ImGui::End();
		return;
	}
	const char* state = !playing ? "停止中" : editor->isPaused() ? "一時停止中" : "再生中";
	if (playing) ImGui::TextColored(PLAY_TEXT, "%s", state);
	else ImGui::TextUnformatted(state);
	ImGui::SameLine(0, 20);
	if (replayRun) ImGui::Text("フレーム %d / %d", editor->getPlayFrame(), replayRun->replay.length);
	else ImGui::Text("フレーム %d", editor->getPlayFrame());
	if (recorder)
	{
		ImGui::SameLine(0, 20);
		ImGui::TextColored(COLOR_RECORD, "● 記録中");
	}
	ImGui::SameLine(0, 20);
	ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);
	ImGui::SameLine(0, 20);
	ImGui::Text("選択: %s", selected >= 0 ? editor->displayName(selectedEntity()).c_str() : "なし");
	ImGui::SameLine(0, 20);
	if (editor->isDirty())
	{
		refreshChanges();
		std::string text = changes.empty() ? "未保存" : "未保存 " + std::to_string(changes.size()) + " 件";
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARNING);
		if (ImGui::Selectable(text.c_str(), false, 0, ImVec2(ImGui::CalcTextSize(text.c_str()).x, 0))) focusChanges = true;
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("押すと差分を開く");
	}
	else ImGui::TextDisabled("保存済み");
	if (editor->hasSceneConflict())
	{
		ImGui::SameLine(0, 20);
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARNING);
		if (ImGui::Selectable("外でファイルが変わりました", false, 0, ImVec2(ImGui::CalcTextSize("外でファイルが変わりました").x, 0))) openFileConflict = true;
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("押すと読み直すか決める");
	}

	//最後の警告・エラー(押すとログを開く)
	if (!lastProblem.empty())
	{
		std::string text = (lastProblemLevel == LogLevel::Error ? "エラー: " : "警告: ") + lastProblem;
		float w = ImGui::CalcTextSize(text.c_str()).x;
		ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 20, ImGui::GetWindowWidth() - w - 16));
		ImVec4 color = problemSeen ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : (lastProblemLevel == LogLevel::Error ? COLOR_ERROR : COLOR_WARNING);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		if (ImGui::Selectable(text.c_str(), false, 0, ImVec2(w, 0))) focusLog = true;
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("押すとログを開く");
	}
	ImGui::End();
}

void EditorApp::drawHierarchy()
{
	ImGui::Begin(WIN_HIERARCHY);
	bool playing = editor->isPlaying();
	ImGui::BeginDisabled(playing);
	if (ImGui::Button("+ 空の Entity"))
	{
		editor->createEntity("Entity");
		selected = static_cast<int>(editor->entities().size()) - 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("+ プレハブ")) ImGui::OpenPopup("prefabs");
	if (ImGui::BeginPopup("prefabs"))
	{
		for (auto& p : assetsIn("prefabs", { ".json" }))
		{
			if (ImGui::Selectable(p.c_str()))
			{
				auto made = editor->createFromPrefab(p);
				if (made.empty()) log("プレハブを置けませんでした: " + p, LogLevel::Error);
				else selected = static_cast<int>(editor->entities().size()) - static_cast<int>(made.size());
			}
		}
		ImGui::EndPopup();
	}
	ImGui::EndDisabled();
	ImGui::Separator();

	if (focusFilter)
	{
		ImGui::SetKeyboardFocusHere();
		focusFilter = false;
	}
	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##filter", "絞り込み (Ctrl+F)", &hierarchyFilter);

	ImGui::BeginChild("list");
	auto list = editor->entities();
	auto names = editor->displayNames();
	auto lower = [](std::string s) {
		for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		return s;
	};
	std::string filter = lower(hierarchyFilter);
	if (renaming >= static_cast<int>(list.size()) || playing) renaming = -1;
	for (int i = 0; i < static_cast<int>(list.size()); i++)
	{
		if (!filter.empty() && lower(names[i]).find(filter) == std::string::npos && i != selected) continue;
		ImGui::PushID(static_cast<int>(list[i].id));
		if (renaming == i)
		{
			if (renameFocus)
			{
				ImGui::SetKeyboardFocusHere();
				renameFocus = false;
			}
			ImGui::SetNextItemWidth(-1);
			bool enter = ImGui::InputText("##rename", &renameText, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if (enter || ImGui::IsItemDeactivatedAfterEdit())
			{
				editor->rename(list[i], renameText);
				renaming = -1;
			}
			else if (ImGui::IsItemDeactivated()) renaming = -1; //Esc
			ImGui::PopID();
			continue;
		}
		std::string label = names[i];
		if (editor->world().getPrefabLink(list[i])) label = "◆ " + label;
		if (ImGui::Selectable(label.c_str(), selected == i, ImGuiSelectableFlags_AllowDoubleClick))
		{
			selected = i;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !playing) startRename(i);
		}
		if (!playing && ImGui::BeginPopupContextItem())
		{
			selected = i;
			if (ImGui::MenuItem("名前を変える", "F2")) startRename(i);
			if (ImGui::MenuItem("プレハブにする...")) runCommand("prefab.saveSelection");
			if (const PrefabLink* link = editor->world().getPrefabLink(list[i]))
			{
				if (ImGui::MenuItem(("プレハブを開いて直す (" + link->prefab + ")").c_str())) runCommand("prefab.edit");
			}
			if (ImGui::MenuItem("JSON で写す", "Ctrl+C")) runCommand("entity.copy");
			if (ImGui::MenuItem("複製", "Ctrl+D"))
			{
				editor->duplicate(list[i]);
				selected = static_cast<int>(editor->entities().size()) - 1;
			}
			if (ImGui::MenuItem("削除", "Delete"))
			{
				editor->destroy(list[i]);
				selected = -1;
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}
	//アセット欄からプレハブを落とすと置く
	ImGui::Dummy(ImMax(ImGui::GetContentRegionAvail(), ImVec2(1, 40)));
	if (!playing && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD))
		{
			std::string id(static_cast<const char*>(p->Data));
			if (id.rfind("prefabs/", 0) == 0 && !editor->createFromPrefab(id).empty()) selected = static_cast<int>(editor->entities().size()) - 1;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::EndChild();
	ImGui::End();
}

namespace
{
	/// <summary>
	/// x/y/z を横に並べ、左端に軸の色(赤・緑・青)の線を付けたドラッグ欄
	/// </summary>
	bool dragVector(float* v, int n, float speed, bool& activated)
	{
		static const ImU32 colors[3] = { IM_COL32(225, 85, 80, 255), IM_COL32(115, 190, 80, 255), IM_COL32(80, 140, 235, 255) };
		const float spacing = 4;
		float w = (ImGui::GetContentRegionAvail().x - spacing * (n - 1)) / n;
		bool changed = false;
		for (int i = 0; i < n; i++)
		{
			if (i) ImGui::SameLine(0, spacing);
			ImGui::PushID(i);
			ImGui::SetNextItemWidth(w);
			changed |= ImGui::DragFloat("##c", &v[i], speed, 0, 0, "%.3f");
			activated |= ImGui::IsItemActivated();
			ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddRectFilled(mn, ImVec2(mn.x + 3, mx.y), colors[i]);
			ImGui::PopID();
		}
		return changed;
	}
}

bool EditorApp::drawField(Entity e, const ComponentInfo& info, const FieldInfo& f)
{
	Json v = editor->getField(e, info.name, f.name);
	const char* label = "##v";
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool changed = false;
	Json next;
	auto startEdit = [&]() { if (ImGui::IsItemActivated()) editor->recordUndo(); };

	switch (f.kind)
	{
	case FieldKind::Bool:
	{
		bool b = v.get<bool>();
		if (ImGui::Checkbox(label, &b)) { editor->recordUndo(); changed = true; next = b; }
		break;
	}
	case FieldKind::Int:
	{
		int i = v.get<int>();
		changed = ImGui::DragInt(label, &i);
		startEdit();
		next = i;
		break;
	}
	case FieldKind::Float:
	{
		float x = v.get<float>();
		changed = ImGui::DragFloat(label, &x, 0.05f, 0, 0, "%.3f");
		startEdit();
		next = x;
		break;
	}
	case FieldKind::Vec2:
	{
		float a[2] = { v[0].get<float>(), v[1].get<float>() };
		bool activated = false;
		changed = dragVector(a, 2, 0.05f, activated);
		if (activated) editor->recordUndo();
		next = Json::array({ a[0], a[1] });
		break;
	}
	case FieldKind::Vec3:
	{
		float a[3] = { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
		float speed = f.name == "rotation" ? 0.5f : 0.05f;
		bool activated = false;
		changed = dragVector(a, 3, speed, activated);
		if (activated) editor->recordUndo();
		next = Json::array({ a[0], a[1], a[2] });
		break;
	}
	case FieldKind::String:
	{
		std::string s = v.get<std::string>();
		bool isAsset = f.name == "path" || f.name == "asset" || f.name == "texture" || f.name == "mesh";
		if (isAsset) ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
		changed = ImGui::InputText(label, &s);
		startEdit();
		next = s;
		//アセット欄からドラッグして落とす
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD))
			{
				editor->recordUndo();
				next = std::string(static_cast<const char*>(p->Data));
				changed = true;
			}
			ImGui::EndDragDropTarget();
		}
		if (isAsset)
		{
			ImGui::SameLine(0, 2);
			if (ImGui::Button("...")) ImGui::OpenPopup("pick");
			if (ImGui::BeginPopup("pick"))
			{
				std::vector<std::string> choices;
				if (f.name == "mesh") choices = { "builtin:cube", "builtin:plane", "builtin:sphere" };
				auto files = f.name == "mesh" ? assetsIn("models", { ".gltf", ".glb" }) : assetsIn("images", {});
				choices.insert(choices.end(), files.begin(), files.end());
				if (f.name == "texture") choices.insert(choices.begin(), "");
				for (auto& c : choices)
				{
					if (ImGui::Selectable(c.empty() ? "(なし)" : c.c_str(), c == s))
					{
						editor->recordUndo();
						next = c;
						changed = true;
					}
				}
				ImGui::EndPopup();
			}
		}
		break;
	}
	case FieldKind::Enum:
	{
		std::string cur = v.get<std::string>();
		if (ImGui::BeginCombo(label, cur.c_str()))
		{
			for (auto& n : f.enumNames)
			{
				if (ImGui::Selectable(n.c_str(), n == cur) && n != cur)
				{
					editor->recordUndo();
					next = n;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		break;
	}
	case FieldKind::Flags:
	{
		Json names = v;
		bool firstFlag = true;
		const float cellWidth = ImGui::GetContentRegionAvail().x;
		float lineUsed = 0;
		for (auto& n : f.enumNames)
		{
			if (n == "NONE") continue;
			bool on = std::find(v.begin(), v.end(), Json(n)) != v.end();
			//入りきらなければ折り返す
			float need = ImGui::CalcTextSize(n.c_str()).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetStyle().ItemSpacing.x;
			if (!firstFlag && lineUsed + need <= cellWidth) ImGui::SameLine();
			else lineUsed = 0;
			lineUsed += need;
			firstFlag = false;
			if (ImGui::Checkbox(n.c_str(), &on))
			{
				editor->recordUndo();
				names = Json::array();
				for (auto& m : f.enumNames)
				{
					bool mOn = m == n ? on : std::find(v.begin(), v.end(), Json(m)) != v.end();
					if (mOn && m != "NONE") names.push_back(m);
				}
				next = names;
				changed = true;
			}
		}
		break;
	}
	case FieldKind::EnumMap:
	{
		//キー(左)→ 値(右)の表。値は絞り込める選択、× で外す、下の欄で足す
		Json map = v.is_object() ? v : Json::object();
		std::string removeKey;
		if (ImGui::BeginTable("##map", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
			for (auto& key : f.keyNames) //表示はキーの決まった順
			{
				if (!map.contains(key)) continue;
				ImGui::PushID(key.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(key.c_str());
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				std::string cur = map[key].is_string() ? map[key].get<std::string>() : "";
				std::string shown = cur.rfind("KEY_", 0) == 0 ? cur.substr(4) : cur;
				ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX));
				if (ImGui::BeginCombo("##value", shown.c_str(), ImGuiComboFlags_HeightLarge))
				{
					if (ImGui::IsWindowAppearing())
					{
						enumFilter.clear();
						ImGui::SetKeyboardFocusHere();
					}
					ImGui::SetNextItemWidth(-FLT_MIN);
					ImGui::InputTextWithHint("##filter", "絞り込み", &enumFilter);
					std::string lowerFilter = enumFilter;
					for (auto& ch : lowerFilter) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
					for (auto& n : f.enumNames)
					{
						if (!lowerFilter.empty() && n.find(lowerFilter) == std::string::npos) continue;
						if (ImGui::Selectable(n.rfind("KEY_", 0) == 0 ? n.c_str() + 4 : n.c_str(), n == cur) && n != cur)
						{
							map[key] = n;
							changed = true;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::TableNextColumn();
				if (ImGui::Button("x", ImVec2(ImGui::GetFrameHeight(), 0))) removeKey = key;
				ImGui::SetItemTooltip("%s を外す", key.c_str());
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (!removeKey.empty())
		{
			map.erase(removeKey);
			changed = true;
		}
		//まだ無いキーを足す
		std::vector<std::string> missing;
		for (auto& key : f.keyNames) if (!map.contains(key)) missing.push_back(key);
		if (!missing.empty())
		{
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##add", "+ 足す", ImGuiComboFlags_None))
			{
				for (auto& key : missing)
				{
					if (ImGui::Selectable(key.c_str()))
					{
						map[key] = f.enumNames.empty() ? Json() : Json(f.enumNames.front());
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
		}
		if (changed)
		{
			editor->recordUndo();
			next = map;
		}
		break;
	}
	case FieldKind::Other:
	{
		//型の決まった欄が無いもの(キー割り当ての表など)は JSON のまま直す
		std::string text = v.dump(1);
		ImGui::InputTextMultiline(label, &text, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 6));
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			try
			{
				next = Json::parse(text);
				editor->recordUndo();
				changed = true;
			}
			catch (const std::exception& ex)
			{
				log(info.name + "." + f.name + ": JSON の書き方が違います: " + ex.what(), LogLevel::Error);
			}
		}
		break;
	}
	}

	if (changed)
	{
		std::string error;
		if (!editor->setField(e, info.name, f.name, next, &error)) log(info.name + "." + f.name + ": " + error, LogLevel::Error);
	}
	return changed;
}

void EditorApp::drawFieldRows(Entity e, const ComponentInfo& info, bool runtime)
{
	auto& cm = editor->world().getComponentManager();
	if (!ImGui::BeginTable("fields", 2, ImGuiTableFlags_SizingStretchProp)) return;
	ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 112);
	ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
	for (auto& f : info.fields)
	{
		if (f.runtime != runtime) continue;
		ImGui::PushID(f.name.c_str());
		ImGui::TableNextRow();

		//左: 名前。プレハブと違う値なら青くし、右クリックでプレハブの値に戻せる
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		Json prefabValue = runtime ? Json() : editor->prefabField(e, info.name, f.name);
		bool overridden = !prefabValue.is_null() && prefabValue != editor->getField(e, info.name, f.name);
		if (overridden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.7f, 1.0f, 1.0f));
		ImGui::Selectable(f.name.c_str(), false, ImGuiSelectableFlags_AllowOverlap);
		if (overridden) ImGui::PopStyleColor();
		if (overridden)
		{
			ImGui::SetItemTooltip("プレハブと違う値(保存すると overrides に書かれる)\nプレハブの値: %s\n右クリックでプレハブの値に戻す", prefabValue.dump().c_str());
			if (ImGui::BeginPopupContextItem("revert"))
			{
				if (ImGui::MenuItem("プレハブの値に戻す")) editor->revertToPrefab(e, info.name, f.name);
				ImGui::EndPopup();
			}
		}

		//右: 値
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginDisabled(runtime);
		drawField(e, info, f);
		ImGui::EndDisabled();
		ImGui::PopID();
		if (!info.has(cm, e)) break; //編集でシーンが作り直された
	}
	ImGui::EndTable();
}

void EditorApp::drawInspector()
{
	ImGui::Begin(WIN_INSPECTOR);
	Entity e = selectedEntity();
	if (selected < 0)
	{
		ImGui::TextDisabled("ヒエラルキーで Entity を選んでください");
		ImGui::End();
		return;
	}
	ImGui::Text("%s", editor->displayName(e).c_str());
	if (const PrefabLink* link = editor->world().getPrefabLink(e))
	{
		ImGui::TextDisabled("プレハブ: %s [%d]", link->prefab.c_str(), link->index);
	}
	ImGui::Separator();

	auto& cm = editor->world().getComponentManager();
	bool playChanged = editor->isPlaying() != inspectorPlaying;
	inspectorPlaying = editor->isPlaying();
	std::string toRemove;
	for (auto& info : ComponentRegistry::getInstance().all())
	{
		if (!info.has(cm, e)) continue;
		ImGui::PushID(info.name.c_str());
		bool open = true;
		if (ImGui::CollapsingHeader(info.name.c_str(), &open, ImGuiTreeNodeFlags_DefaultOpen))
		{
			drawFieldRows(e, info, false);
			bool hasRuntime = std::any_of(info.fields.begin(), info.fields.end(), [](const FieldInfo& f) { return f.runtime; });
			if (hasRuntime && info.has(cm, e))
			{
				//再生を始めたら開き、止めたら閉じる(その間は手で開け閉めできる)
				if (playChanged) ImGui::SetNextItemOpen(editor->isPlaying(), ImGuiCond_Always);
				if (ImGui::TreeNodeEx("実行時の値", ImGuiTreeNodeFlags_SpanAvailWidth))
				{
					ImGui::SetItemTooltip("ゲームが動いている間に変わる値。ここでは読むだけ");
					drawFieldRows(e, info, true);
					ImGui::TreePop();
				}
			}
		}
		if (!open) toRemove = info.name;
		ImGui::PopID();
	}
	if (!toRemove.empty()) editor->removeComponent(e, toRemove);

	ImGui::Spacing();
	if (ImGui::Button("コンポーネントを追加", ImVec2(-1, 0))) ImGui::OpenPopup("add");
	if (ImGui::BeginPopup("add"))
	{
		for (auto& info : ComponentRegistry::getInstance().all())
		{
			if (info.has(cm, e)) continue;
			if (ImGui::Selectable(info.name.c_str())) editor->addComponent(e, info.name);
		}
		ImGui::EndPopup();
	}
	ImGui::End();
}

void EditorApp::drawSceneView()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin(WIN_SCENE);
	ImGui::PopStyleVar();

	//ゲームの画面を縦横比を保って真ん中に出す
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float scale = std::min(avail.x / gameWidth, avail.y / gameHeight);
	if (scale <= 0) scale = 0.01f;
	viewSize = ImVec2(gameWidth * scale, gameHeight * scale);
	ImVec2 cursor = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(cursor.x + (avail.x - viewSize.x) * 0.5f, cursor.y + (avail.y - viewSize.y) * 0.5f));
	viewMin = ImGui::GetCursorScreenPos();
	ImTextureID tex = static_cast<ImTextureID>(drawer.renderTargetTexture(gameTarget));
	ImGui::Image(ImTextureRef(tex), viewSize, ImVec2(0, 1), ImVec2(1, 0)); //描き先は上下逆さま
	viewHovered = ImGui::IsItemHovered();
	if (editor->isPlaying())
	{
		//再生中は画面のまわりを色で囲む
		ImVec2 max(viewMin.x + viewSize.x, viewMin.y + viewSize.y);
		ImGui::GetWindowDrawList()->AddRect(ImVec2(viewMin.x - 2, viewMin.y - 2), ImVec2(max.x + 2, max.y + 2), ImGui::GetColorU32(PLAY_TEXT), 0, 0, 3.0f);
	}
	drawGizmo();
	//ギズモを触っていない時の左クリックで、そこにある物を選ぶ
	if (viewHovered && !editor->isPlaying() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
	{
		ImVec2 mouse = ImGui::GetIO().MousePos;
		//今選んでいるスプライトの上なら、それを動かす(重なった物の下を選び直さない)
		Vec2 min, max;
		float sx = viewSize.x / gameWidth, sy = viewSize.y / gameHeight;
		bool onSelected = selected >= 0 && spriteRect(selectedEntity(), min, max)
			&& mouse.x >= viewMin.x + min.x * sx && mouse.x <= viewMin.x + max.x * sx && mouse.y >= viewMin.y + min.y * sy && mouse.y <= viewMin.y + max.y * sy;
		if (!onSelected) pickAt(mouse);
		Vec2 dummyMin, dummyMax;
		if (selected >= 0 && spriteRect(selectedEntity(), dummyMin, dummyMax))
		{
			drag2D = true;
			drag2DMoved = false;
			dragMouseStart = mouse;
			Json p = editor->getField(selectedEntity(), "Transform", "position");
			dragPosStart = Vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
		}
	}
	if (!editor->isPlaying()) drawSelection2D();
	else drag2D = false;
	ImGui::End();
}

glm::mat4 EditorApp::viewProjection() const
{
	return glm::perspective(glm::radians(viewFov), static_cast<float>(gameWidth) / gameHeight, viewNear, viewFar) * camera.view();
}

void EditorApp::drawGizmo()
{
	bool using3D = view3D && !editor->isPlaying();
	Entity e = selectedEntity();
	if (!using3D || selected < 0 || !editor->world().getComponentManager().has<TransformComponent>(e))
	{
		gizmoWasUsing = false;
		return;
	}
	auto& t = editor->world().getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
	glm::mat4 view = camera.view();
	glm::mat4 proj = glm::perspective(glm::radians(viewFov), static_cast<float>(gameWidth) / gameHeight, viewNear, viewFar);
	glm::mat4 model = MathUtil::modelMatrix(t.position, t.rotation, t.scale);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(viewMin.x, viewMin.y, viewSize.x, viewSize.y);
	ImGuizmo::OPERATION op = gizmoOperation == 0 ? ImGuizmo::TRANSLATE : gizmoOperation == 1 ? ImGuizmo::ROTATE : ImGuizmo::SCALE;
	ImGuizmo::MODE mode = gizmoOperation == 2 ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
	bool changed = ImGuizmo::Manipulate(&view[0][0], &proj[0][0], op, mode, &model[0][0]);
	bool usingNow = ImGuizmo::IsUsing();
	if (usingNow && !gizmoWasUsing) editor->recordUndo();
	gizmoWasUsing = usingNow;
	if (!changed) return;

	//行列を 位置・回転(度、Y→X→Z)・拡大 に戻す
	Vec3 position, rotation, scale;
	MathUtil::decompose(model, position, rotation, scale);
	auto entity = selectedEntity();
	editor->setField(entity, "Transform", "position", toJson(position));
	if (gizmoOperation == 1) editor->setField(entity, "Transform", "rotation", toJson(rotation));
	if (gizmoOperation == 2) editor->setField(entity, "Transform", "scale", toJson(scale));
}

void EditorApp::pickAt(const ImVec2& mouse)
{
	if (viewSize.x <= 0) return;
	float u = (mouse.x - viewMin.x) / viewSize.x;
	float v = (mouse.y - viewMin.y) / viewSize.y;
	auto list = editor->entities();
	auto& cm = editor->world().getComponentManager();
	int best = -1;

	if (view3D)
	{
		//カーソルの下を通る光線と、各メッシュの範囲の箱(モデルの座標)が交わる一番近いもの
		glm::mat4 inv = glm::inverse(viewProjection());
		glm::vec4 nearP = inv * glm::vec4(u * 2 - 1, 1 - v * 2, -1, 1);
		glm::vec4 farP = inv * glm::vec4(u * 2 - 1, 1 - v * 2, 1, 1);
		glm::vec3 origin = glm::vec3(nearP) / nearP.w;
		glm::vec3 dir = glm::vec3(farP) / farP.w - origin;
		float bestT = 1e30f;
		for (int i = 0; i < static_cast<int>(list.size()); i++)
		{
			Entity e = list[i];
			if (!cm.has<MeshRendererComponent>(e) || !cm.has<TransformComponent>(e)) continue;
			glm::vec3 lo, hi;
			if (!ResourceManager::getInstance().getMeshBounds(cm.getComponentArray<MeshRendererComponent>().getComponent(e).mesh, lo, hi)) continue;
			auto& t = cm.getComponentArray<TransformComponent>().getComponent(e);
			glm::mat4 toLocal = glm::inverse(MathUtil::modelMatrix(t.position, t.rotation, t.scale));
			glm::vec3 o = glm::vec3(toLocal * glm::vec4(origin, 1));
			glm::vec3 d = glm::vec3(toLocal * glm::vec4(dir, 0));
			//平らな面(plane)でも当たるよう、厚さ 0 の軸を少しふくらませる
			for (int k = 0; k < 3; k++) if (hi[k] - lo[k] < 1e-4f) { lo[k] -= 0.01f; hi[k] += 0.01f; }
			float t0 = 0, t1 = 1e30f;
			bool hit = true;
			for (int k = 0; k < 3 && hit; k++)
			{
				if (std::abs(d[k]) < 1e-9f)
				{
					hit = o[k] >= lo[k] && o[k] <= hi[k];
					continue;
				}
				float a = (lo[k] - o[k]) / d[k], b = (hi[k] - o[k]) / d[k];
				if (a > b) std::swap(a, b);
				t0 = std::max(t0, a);
				t1 = std::min(t1, b);
				hit = t0 <= t1;
			}
			if (hit && t0 < bestT)
			{
				bestT = t0;
				best = i;
			}
		}
	}
	else
	{
		//2D: スプライトの四角の中で、一番手前(layer → order が大きい)もの
		Vec2 p(u * gameWidth, v * gameHeight);
		std::pair<int, int> top{ -1000000, -1000000 };
		for (int i = 0; i < static_cast<int>(list.size()); i++)
		{
			Entity e = list[i];
			Vec2 min, max;
			if (!spriteRect(e, min, max)) continue;
			auto& r = cm.getComponentArray<RendererComponent>().getComponent(e);
			if (p.x < min.x || p.y < min.y || p.x > max.x || p.y > max.y) continue;
			std::pair<int, int> key{ r.layer, r.order };
			if (key >= top)
			{
				top = key;
				best = i;
			}
		}
	}
	selected = best;
}

bool EditorApp::spriteRect(Entity e, Vec2& min, Vec2& max)
{
	auto& cm = editor->world().getComponentManager();
	if (!editor->world().alive(e) || !cm.has<RendererComponent>(e) || !cm.has<TransformComponent>(e)) return false;
	auto& r = cm.getComponentArray<RendererComponent>().getComponent(e);
	auto& t = cm.getComponentArray<TransformComponent>().getComponent(e);
	Vec2 size = AssetDatabase::getInstance().imageSize(r.path);
	if (size.x <= 0 || size.y <= 0) size = Vec2(32, 32); //画像が読めなくても選べるように
	min = Vec2(t.position.x, t.position.y);
	if (r.useCamera) min -= editor->world().getCamera().position;
	max = min + Vec2(size.x * t.scale.x, size.y * t.scale.y);
	return true;
}

void EditorApp::drawSelection2D()
{
	Entity e = selectedEntity();
	Vec2 min, max;
	bool sprite = selected >= 0 && spriteRect(e, min, max);
	ImGuiIO& io = ImGui::GetIO();
	float sx = viewSize.x / gameWidth, sy = viewSize.y / gameHeight;

	//ドラッグ中: 動いた分を位置に足す(最初に動いた時に元に戻す記録)
	if (drag2D)
	{
		if (!sprite || !ImGui::IsMouseDown(ImGuiMouseButton_Left) || editor->isPlaying())
		{
			drag2D = false;
		}
		else
		{
			ImVec2 d(io.MousePos.x - dragMouseStart.x, io.MousePos.y - dragMouseStart.y);
			if (!drag2DMoved && (std::abs(d.x) > 2 || std::abs(d.y) > 2))
			{
				editor->recordUndo();
				drag2DMoved = true;
				e = selectedEntity();
			}
			if (drag2DMoved)
			{
				Vec3 pos = dragPosStart;
				pos.x += d.x / sx;
				pos.y += d.y / sy;
				if (!io.KeyShift) //Shift を押している間は 1px 単位にしない
				{
					pos.x = std::round(pos.x);
					pos.y = std::round(pos.y);
				}
				editor->setField(e, "Transform", "position", toJson(pos));
				spriteRect(e, min, max);
			}
		}
	}

	if (!sprite) return;
	ImVec2 a(viewMin.x + min.x * sx, viewMin.y + min.y * sy);
	ImVec2 b(viewMin.x + max.x * sx, viewMin.y + max.y * sy);
	auto* draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(viewMin, ImVec2(viewMin.x + viewSize.x, viewMin.y + viewSize.y), true);
	draw->AddRect(ImVec2(a.x - 1, a.y - 1), ImVec2(b.x + 1, b.y + 1), IM_COL32(0, 0, 0, 160), 0, 0, 3.0f);
	draw->AddRect(ImVec2(a.x - 1, a.y - 1), ImVec2(b.x + 1, b.y + 1), IM_COL32(255, 200, 60, 255), 0, 0, 1.5f);
	//左上(位置の基準点)に小さな四角
	draw->AddRectFilled(ImVec2(a.x - 3, a.y - 3), ImVec2(a.x + 3, a.y + 3), IM_COL32(255, 200, 60, 255));
	draw->PopClipRect();
	bool over = io.MousePos.x >= a.x && io.MousePos.x <= b.x && io.MousePos.y >= a.y && io.MousePos.y <= b.y;
	if (viewHovered && over && !editor->isPlaying()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
}

void EditorApp::drawAssets()
{
	ImGui::Begin(WIN_ASSETS);
	refreshAssets();
	ImGui::TextDisabled("%s", AssetDatabase::getInstance().getRoot().generic_string().c_str());
	ImGui::TextDisabled("ドラッグしてインスペクタの欄やヒエラルキーへ。シーンはダブルクリックで開く");
	ImGui::Separator();
	ImGui::BeginChild("files");
	std::string currentFolder;
	bool folderOpen = true;
	for (auto& f : assetFiles)
	{
		std::string folder = f.find('/') == std::string::npos ? "" : f.substr(0, f.find('/'));
		if (folder != currentFolder)
		{
			if (!currentFolder.empty() && folderOpen) ImGui::TreePop();
			currentFolder = folder;
			folderOpen = folder.empty() || ImGui::TreeNodeEx(folder.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
			if (folder.empty()) currentFolder.clear();
		}
		if (!folderOpen) continue;
		std::string leaf = folder.empty() ? f : f.substr(folder.size() + 1);
		ImGui::PushID(f.c_str());
		ImGui::Selectable(leaf.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (f.rfind("scenes/", 0) == 0 && !editor->isPlaying()) request(Pending::OpenScene, f);
			if (f.rfind("prefabs/", 0) == 0 && !editor->isPlaying()) editPrefab(f);
		}
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(ASSET_PAYLOAD, f.c_str(), f.size() + 1);
			ImGui::TextUnformatted(f.c_str());
			ImGui::EndDragDropSource();
		}
		ImGui::PopID();
	}
	if (!currentFolder.empty() && folderOpen) ImGui::TreePop();
	ImGui::EndChild();
	ImGui::End();
}

void EditorApp::drawSystems()
{
	ImGui::Begin(WIN_SYSTEMS);
	ImGui::TextDisabled("上から順に毎フレーム動く");
	auto names = editor->systems();
	bool playing = editor->isPlaying();
	int move = -1, dir = 0, remove = -1;
	for (int i = 0; i < static_cast<int>(names.size()); i++)
	{
		ImGui::PushID(i);
		ImGui::BeginDisabled(playing);
		if (ImGui::ArrowButton("up", ImGuiDir_Up)) { move = i; dir = -1; }
		ImGui::SameLine(0, 2);
		if (ImGui::ArrowButton("down", ImGuiDir_Down)) { move = i; dir = 1; }
		ImGui::SameLine(0, 2);
		if (ImGui::SmallButton("x")) remove = i;
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextUnformatted(names[i].c_str());
		ImGui::PopID();
	}
	if (move >= 0 && move + dir >= 0 && move + dir < static_cast<int>(names.size()))
	{
		std::swap(names[move], names[move + dir]);
		editor->setSystems(names);
	}
	if (remove >= 0)
	{
		names.erase(names.begin() + remove);
		editor->setSystems(names);
	}
	ImGui::BeginDisabled(playing);
	if (ImGui::BeginCombo("##add", "System を追加"))
	{
		for (auto& n : SystemRegistry::getInstance().names())
		{
			if (ImGui::Selectable(n.c_str()))
			{
				names.push_back(n);
				editor->setSystems(names);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
	ImGui::End();
}

void EditorApp::drawLog()
{
	ImGui::Begin(WIN_LOG);
	if (ImGui::IsWindowAppearing() || ImGui::IsWindowFocused()) problemSeen = true;
	if (ImGui::SmallButton("消す"))
	{
		logLines.clear();
		lastProblem.clear();
	}
	ImGui::Separator();
	for (auto& l : logLines)
	{
		if (l.level == LogLevel::Info) ImGui::TextUnformatted(l.text.c_str());
		else ImGui::TextColored(l.level == LogLevel::Error ? COLOR_ERROR : COLOR_WARNING, "%s", l.text.c_str());
	}
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
	ImGui::End();
}

void EditorApp::drawPopups()
{
	if (pending != Pending::None && editor->isDirty() && !ImGui::IsPopupOpen("保存されていない変更")) ImGui::OpenPopup("保存されていない変更");
	if (ImGui::BeginPopupModal("保存されていない変更", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%s に保存していない変更があります。", editor->getPath().empty() ? "新しいシーン" : editor->getPath().c_str());
		if (ImGui::Button("保存する"))
		{
			saveScene();
			if (!editor->isDirty()) doPending();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("保存しない"))
		{
			doPending();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("やめる"))
		{
			pending = Pending::None;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (openFileConflict && !ImGui::IsPopupOpen(POPUP_CONFLICT))
	{
		ImGui::OpenPopup(POPUP_CONFLICT);
		openFileConflict = false;
	}
	ImGui::SetNextWindowSizeConstraints(ImVec2(560, 0), ImVec2(900, 600));
	if (ImGui::BeginPopupModal(POPUP_CONFLICT, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%s が外で書き換えられました。エディタには保存していない変更があります。", editor->getPath().c_str());
		ImGui::TextDisabled("外で入った変更: %d 件", static_cast<int>(fileChangeList.size()));
		float rows = static_cast<float>(std::min<size_t>(fileChangeList.size(), 10) + 1);
		ImGui::BeginChild("##outside", ImVec2(820, rows * (ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2) + ImGui::GetStyle().WindowPadding.y * 2 + 4), ImGuiChildFlags_Borders);
		drawChangeTable("##outsideTable", fileChangeList, "前", "ファイル", false);
		ImGui::EndChild();
		if (ImGui::Button("読み直す"))
		{
			reloadScene();
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("エディタの変更を捨ててファイルを読む(Ctrl+Z で戻せる)");
		ImGui::SameLine();
		if (ImGui::Button("このまま続ける"))
		{
			editor->keepEdits();
			refreshChanges(true);
			log("外の変更を読まずに続けます。保存すると " + editor->getPath() + " を上書きします", LogLevel::Warning);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("保存するとファイルを上書きする(差分は「保存したら変わる所」になる)");
		ImGui::SameLine();
		if (ImGui::Button("あとで"))
		{
			focusChanges = true;
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("今は決めない(状態バーと差分タブから決められる)");
		ImGui::EndPopup();
	}

	if (openPrefabSave)
	{
		ImGui::OpenPopup("プレハブにする");
		openPrefabSave = false;
	}
	if (ImGui::BeginPopupModal("プレハブにする", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("選んだ Entity をプレハブのファイルに書き、この Entity をその配置にする");
		ImGui::TextDisabled("assets フォルダからの場所(prefabs/ の下)");
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		ImGui::SetNextItemWidth(360);
		bool enter = ImGui::InputText("##prefabPath", &prefabSavePath, ImGuiInputTextFlags_EnterReturnsTrue);
		bool exists = AssetDatabase::getInstance().exists(prefabSavePath);
		if (exists) ImGui::TextColored(COLOR_WARNING, "同じ名前のファイルがあります。上書きします");
		if (ImGui::Button(exists ? "上書きして保存" : "保存") || enter)
		{
			savePrefab(prefabSavePath);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("やめる")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (openRecordSave)
	{
		ImGui::OpenPopup("記録を保存");
		openRecordSave = false;
	}
	if (ImGui::BeginPopupModal("記録を保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%d フレーム(%.1f 秒)、入力の変化 %d 回、確認点 %d か所", pendingRecord.length, pendingRecord.length / static_cast<float>(pendingRecord.fps),
			static_cast<int>(pendingRecord.events.size()), static_cast<int>(pendingRecord.checkpoints.size()));
		ImGui::TextUnformatted("テストの名前(tests/replays/<名前>.json に保存)");
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		ImGui::SetNextItemWidth(360);
		bool enter = ImGui::InputText("##name", &recordName, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::Checkbox("続けて基準(確認点の画面と状態)を作る", &recordThenUpdate);
		bool exists = replayFiles.count(recordName) > 0;
		if (exists) ImGui::TextColored(COLOR_WARNING, "同じ名前のテストを上書きします");
		if ((ImGui::Button("保存") || enter) && !recordName.empty())
		{
			try
			{
				pendingRecord.save(replayDir() / (recordName + ".json"));
				log("操作の記録を保存しました: " + (replayDir() / (recordName + ".json")).generic_string());
				refreshReplays(true);
				if (recordThenUpdate) startReplay(recordName, ReplayCheck::Mode::Update, true);
			}
			catch (const std::exception& e)
			{
				log(std::string("記録を保存できませんでした: ") + e.what(), LogLevel::Error);
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("捨てる")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (openSaveAs)
	{
		ImGui::OpenPopup("名前を付けて保存");
		openSaveAs = false;
	}
	if (ImGui::BeginPopupModal("名前を付けて保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("assets フォルダからの場所(例: scenes/Stage1.json)");
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		ImGui::SetNextItemWidth(360);
		bool enter = ImGui::InputText("##path", &saveAsPath, ImGuiInputTextFlags_EnterReturnsTrue);
		if (ImGui::Button("保存") || enter)
		{
			std::string error;
			if (editor->saveAs(saveAsPath, error))
			{
				log("保存しました: " + saveAsPath);
				refreshAssets(true);
				if (pending != Pending::None) doPending();
			}
			else log("保存できませんでした: " + error, LogLevel::Error);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("やめる")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void EditorApp::buildDefaultLayout(ImGuiID dockspace)
{
	ImGui::DockBuilderRemoveNode(dockspace);
	ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);
	ImGuiID center = dockspace;
	ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.17f, nullptr, &center);
	ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
	ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.27f, nullptr, &center);
	ImGui::DockBuilderDockWindow(WIN_HIERARCHY, left);
	ImGui::DockBuilderDockWindow(WIN_INSPECTOR, right);
	ImGui::DockBuilderDockWindow(WIN_ASSETS, bottom);
	ImGui::DockBuilderDockWindow(WIN_SYSTEMS, bottom);
	ImGui::DockBuilderDockWindow(WIN_LOG, bottom);
	ImGui::DockBuilderDockWindow(WIN_REPLAYS, bottom);
	ImGui::DockBuilderDockWindow(WIN_CHANGES, bottom);

	ImGui::DockBuilderDockWindow(WIN_SCENE, center);
	ImGui::DockBuilderFinish(dockspace);
}

void EditorApp::handleShortcuts()
{
	if (ImGui::GetIO().WantTextInput) return;
	for (auto& c : commands)
	{
		if (!c.chord || c.choices || !ImGui::IsKeyChordPressed(c.chord)) continue;
		if (c.shortcutWhen ? c.shortcutWhen() : c.isEnabled()) c.run("");
	}
}

void EditorApp::updateCamera()
{
	if (!viewHovered || editor->isPlaying() || !useEditorCamera) return;
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
	if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) camera.pan(io.MouseDelta.x, io.MouseDelta.y);
	if (io.MouseWheel != 0) camera.zoom(io.MouseWheel);
}

void EditorApp::applyScriptedInput(int frame)
{
	ImGuiIO& io = ImGui::GetIO();
	auto click = debug.clickAt(frame);
	if (click.move) io.AddMousePosEvent(click.pos.x, click.pos.y);
	if (click.press) io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	if (click.release) io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	for (auto& text : debug.typedAt(frame))
	{
		if (text == "{ENTER}")
		{
			io.AddKeyEvent(ImGuiKey_Enter, true);
			io.AddKeyEvent(ImGuiKey_Enter, false);
		}
		else if (text == "{SELECTALL}")
		{
			io.AddKeyEvent(ImGuiMod_Ctrl, true);
			io.AddKeyEvent(ImGuiKey_A, true);
			io.AddKeyEvent(ImGuiKey_A, false);
			io.AddKeyEvent(ImGuiMod_Ctrl, false);
		}
		else if (text.size() > 2 && text.front() == '{' && text.back() == '}')
		{
			//{CTRL+SHIFT+P} {DOWN} {ESC} のようなキー
			std::string chord = text.substr(1, text.size() - 2);
			std::vector<ImGuiKey> mods;
			ImGuiKey key = ImGuiKey_None;
			size_t pos = 0;
			while (pos <= chord.size())
			{
				size_t plus = chord.find('+', pos);
				std::string part = chord.substr(pos, plus == std::string::npos ? std::string::npos : plus - pos);
				pos = plus == std::string::npos ? chord.size() + 1 : plus + 1;
				if (part == "CTRL") mods.push_back(ImGuiMod_Ctrl);
				else if (part == "SHIFT") mods.push_back(ImGuiMod_Shift);
				else if (part == "ALT") mods.push_back(ImGuiMod_Alt);
				else if (part.size() == 1 && part[0] >= 'A' && part[0] <= 'Z') key = static_cast<ImGuiKey>(ImGuiKey_A + (part[0] - 'A'));
				else if (part.size() == 1 && part[0] >= '0' && part[0] <= '9') key = static_cast<ImGuiKey>(ImGuiKey_0 + (part[0] - '0'));
				else if (part.size() >= 2 && part[0] == 'F' && std::isdigit(static_cast<unsigned char>(part[1]))) key = static_cast<ImGuiKey>(ImGuiKey_F1 + std::stoi(part.substr(1)) - 1);
				else if (part == "UP") key = ImGuiKey_UpArrow;
				else if (part == "DOWN") key = ImGuiKey_DownArrow;
				else if (part == "LEFT") key = ImGuiKey_LeftArrow;
				else if (part == "RIGHT") key = ImGuiKey_RightArrow;
				else if (part == "ESC") key = ImGuiKey_Escape;
				else if (part == "DELETE") key = ImGuiKey_Delete;
				else if (part == "TAB") key = ImGuiKey_Tab;
				else if (part == "BACKSPACE") key = ImGuiKey_Backspace;
				else if (part == "SPACE") key = ImGuiKey_Space;
				else std::cerr << "unknown key in --type: " << text << std::endl;
			}
			for (auto m : mods) io.AddKeyEvent(m, true);
			if (key != ImGuiKey_None)
			{
				io.AddKeyEvent(key, true);
				io.AddKeyEvent(key, false);
			}
			for (auto m : mods) io.AddKeyEvent(m, false);
		}
		else io.AddInputCharactersUTF8(text.c_str());
	}
}

// ---- 再現テスト

std::filesystem::path EditorApp::replayDir() const
{
	//assets の隣の tests/replays(ctest もここを流す)
	return AssetDatabase::getInstance().getRoot().parent_path() / "tests" / "replays";
}

void EditorApp::refreshReplays(bool force)
{
	double now = ImGui::GetTime();
	if (!force && now - replaysListedAt < 2.0) return;
	replaysListedAt = now;
	replayFiles.clear();
	std::error_code ec;
	for (auto& entry : std::filesystem::directory_iterator(replayDir(), ec))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
		try
		{
			replayFiles[entry.path().stem().string()] = Replay::load(entry.path());
		}
		catch (const std::exception& e)
		{
			std::string name = entry.path().stem().string();
			if (!replaySummaries.count(name) || replaySummaries[name].lines.empty() || replaySummaries[name].lines[0] != e.what())
			{
				replaySummaries[name] = { false, false, { e.what() } };
				log(std::string("再現テストを読めません: ") + e.what(), LogLevel::Warning);
			}
		}
	}
}

bool EditorApp::canStartReplay(std::string* reason) const
{
	auto fail = [&](const char* text) { if (reason) *reason = text; return false; };
	if (recorder || (replayRun && !replayRun->finished)) return fail("記録・再現テストの途中");
	if (editor->getPath().empty()) return fail("シーンを保存してから(記録は保存したシーンから始める)");
	if (editor->isPrefabMode()) return fail("プレハブを直している間は記録できない(シーンに戻ってから)");
	if (editor->isDirty()) return fail("保存していない変更がある(記録・再現テストはシーンを開き直して始める)");
	return true;
}

void EditorApp::startRecording()
{
	if (!canStartReplay()) return;
	if (editor->isPlaying()) editor->stop();
	//ゲームと同じに、ファイルから読み直したばかりの状態から始める
	std::string error, path = editor->getPath();
	if (!editor->open(path, error))
	{
		log("記録を始められませんでした: " + error, LogLevel::Error);
		return;
	}
	selected = -1;
	const uint32_t seed = 1;
	Random::seed(seed);
	editor->play();
	recorder.emplace(SceneEditor::sceneName(path), 60, seed);
	recordLastFrame = 0;
	log("操作の記録を始めました(■ 停止で終えて保存)");
}

void EditorApp::finishRecording()
{
	if (!recorder) return;
	if (recordLastFrame > 0)
	{
		pendingRecord = recorder->finish(recordLastFrame);
		recordName = pendingRecord.scene;
		for (int i = 2; replayFiles.count(recordName); i++) recordName = pendingRecord.scene + "_" + std::to_string(i);
		openRecordSave = true;
	}
	recorder.reset();
}

void EditorApp::startReplay(const std::string& name, ReplayCheck::Mode mode, bool fast)
{
	if (editor->isPlaying()) editor->stop();
	replayRun.reset();
	std::filesystem::path path = replayDir() / (name + ".json");
	ReplayRun run;
	run.name = name;
	run.fast = fast;
	try
	{
		run.replay = Replay::load(path);
	}
	catch (const std::exception& e)
	{
		log(std::string("再現テストを読めませんでした: ") + e.what(), LogLevel::Error);
		return;
	}
	std::string scene = run.replay.scene;
	if (scene.empty())
	{
		auto& db = AssetDatabase::getInstance();
		if (db.exists("project.json")) scene = db.readJson("project.json").value("startScene", std::string());
	}
	std::string error;
	if (!editor->open(SceneLoader::scenePath(scene), error))
	{
		log("再現テストのシーンを開けませんでした: " + error, LogLevel::Error);
		return;
	}
	selected = -1;
	Random::seed(run.replay.seed);
	run.player.emplace(run.replay);
	run.check.emplace(Replay::expectedDir(path), std::filesystem::temp_directory_path() / "gomi-replay" / name, mode);
	replayRun = std::move(run);
	replaySelected = name;
	editor->play();
	log(std::string(mode == ReplayCheck::Mode::Update ? "基準を作り直します: " : "再現テストを流します: ") + name);
}

void EditorApp::endReplay()
{
	if (!replayRun) return;
	auto& run = *replayRun;
	ReplaySummary summary;
	summary.update = run.check->getMode() == ReplayCheck::Mode::Update;
	summary.passed = run.check->passed(run.replay);
	summary.lines = run.check->report(run.replay, run.name);
	for (auto& line : summary.lines) log(line, summary.passed ? LogLevel::Info : LogLevel::Warning);
	if (!summary.passed && run.restored && !summary.update)
		log("巻き戻した所から流した結果です。覚えた状態に入らない値があると違いが出るので、最初から流して確かめてください", LogLevel::Warning);
	replaySummaries[run.name] = summary;
}

void EditorApp::seekReplay(int frame)
{
	if (!replayRun) return;
	frame = std::clamp(frame, 1, replayRun->replay.length);
	if (frame <= editor->getPlayFrame())
	{
		//戻る時は、その前で一番近い覚えた状態から流し直す(毎回同じ結果になるので、同じ所に着く)
		auto it = replayRun->snapshots.upper_bound(frame);
		if (it != replayRun->snapshots.begin())
		{
			--it;
			auto& s = it->second;
			std::string error;
			if (editor->restorePlayState(s.state, error))
			{
				Random::loadState(s.random);
				replayRun->player = s.player;
				replayRun->check = s.check;
				replayRun->lastChecked = s.lastChecked;
				replayRun->finished = false;
				replayRun->restored = true;
				replayRun->seekTo = frame > it->first ? frame : 0;
				editor->setPaused(frame == it->first);
				return;
			}
			log("覚えた状態に戻せませんでした(最初から流し直します): " + error, LogLevel::Warning);
		}
		std::string name = replayRun->name;
		auto mode = replayRun->check->getMode();
		bool fast = replayRun->fast;
		replayRun.reset();
		editor->stop();
		startReplay(name, mode, fast);
		if (!replayRun) return;
	}
	replayRun->seekTo = frame;
	editor->setPaused(false);
}

int EditorApp::replaySteps() const
{
	if (!replayRun || !editor->isPlaying() || editor->isPaused()) return 1;
	int now = editor->getPlayFrame();
	int left = replayRun->replay.length - now;
	if (left <= 0) return 1;
	if (replayRun->seekTo > now) return std::min({ replayRun->seekTo - now, FAST_STEPS, left });
	return replayRun->fast ? std::min(FAST_STEPS, left) : 1;
}

void EditorApp::afterReplayFrame()
{
	if (!replayRun) return;
	auto& run = *replayRun;
	if (!editor->isPlaying())
	{
		//途中で止めた・ゲームが終わった
		if (!run.finished) endReplay();
		replayRun.reset();
		replayQueue.clear();
		return;
	}
	int frame = editor->getPlayFrame();
	if (frame != run.lastChecked && run.replay.isCheckpoint(frame))
	{
		run.lastChecked = frame;
		Json state = { { "scene", editor->getPlayScene() }, { "frame", frame }, { "world", SceneLoader::save(editor->world()) } };
		auto& result = run.check->checkpoint(frame, state, [&](const std::string& path) { return drawer.captureScreen(path); });
		if (run.check->getMode() == ReplayCheck::Mode::Check && !result.ok()) log(run.name + ": フレーム " + std::to_string(frame) + " が基準と違います", LogLevel::Warning);
	}
	//巻き戻し用に状態を覚える(確認点で比べた後の結果ごと)
	if (frame > 0 && frame % SNAPSHOT_EVERY == 0 && !run.snapshots.count(frame))
	{
		run.snapshots.emplace(frame, ReplayRun::Snapshot{ editor->playState(), Random::saveState(), *run.player, *run.check, run.lastChecked });
	}
	if (run.seekTo > 0 && frame >= run.seekTo)
	{
		run.seekTo = 0;
		editor->setPaused(true);
	}
	if (!run.finished && frame >= run.replay.length)
	{
		run.finished = true;
		endReplay();
		if (!replayQueue.empty())
		{
			std::string next = replayQueue.front();
			replayQueue.erase(replayQueue.begin());
			startReplay(next, ReplayCheck::Mode::Check, true);
		}
		else editor->setPaused(true); //終わりの状態を見られるように止めておく
	}
}

void EditorApp::drawTimeStrip()
{
	auto& run = *replayRun;
	const Replay& r = run.replay;
	int now = editor->getPlayFrame();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = std::max(50.0f, ImGui::GetContentRegionAvail().x);
	float h = 30;
	ImGui::InvisibleButton("strip", ImVec2(w, h));
	bool hovered = ImGui::IsItemHovered();
	auto xOf = [&](int f) { return p.x + 4 + (w - 8) * (r.length > 1 ? static_cast<float>(f - 1) / (r.length - 1) : 1.0f); };
	auto frameAt = [&](float x) { return std::clamp(1 + static_cast<int>(std::round((x - p.x - 4) / (w - 8) * (r.length - 1))), 1, r.length); };

	dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::GetColorU32(ImGuiCol_FrameBg), 3);
	if (now > 0) dl->AddRectFilled(p, ImVec2(xOf(now), p.y + h), ImGui::GetColorU32(ImVec4(PLAY_TINT.x, PLAY_TINT.y, PLAY_TINT.z, 0.6f)), 3);
	//入力が変わったフレーム
	for (auto& e : r.events)
	{
		float x = xOf(e.frame);
		dl->AddLine(ImVec2(x, p.y + h * 0.55f), ImVec2(x, p.y + h - 2), ImGui::GetColorU32(ImGuiCol_TextDisabled));
	}
	//確認点(灰: まだ / 緑: 一致 / 赤: 違う)
	for (int c : r.checkpoints)
	{
		ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
		for (auto& res : run.check->getResults())
		{
			if (res.frame != c) continue;
			color = ImGui::GetColorU32(run.check->getMode() == ReplayCheck::Mode::Update || res.ok() ? COLOR_OK : COLOR_ERROR);
		}
		float x = xOf(c);
		dl->AddTriangleFilled(ImVec2(x - 5, p.y + 2), ImVec2(x + 5, p.y + 2), ImVec2(x, p.y + 11), color);
	}
	float cx = xOf(std::max(now, 1));
	dl->AddLine(ImVec2(cx, p.y), ImVec2(cx, p.y + h), ImGui::GetColorU32(PLAY_TEXT), 2);
	if (hovered)
	{
		int f = frameAt(ImGui::GetIO().MousePos.x);
		ImGui::SetTooltip("フレーム %d(%.2f 秒)\nクリックでそこまで進めて止める\n▼ は確認点、下の線は入力が変わった所", f, f / static_cast<float>(r.fps));
		if (ImGui::IsItemClicked()) seekReplay(f);
	}
	ImGui::Text("フレーム %d / %d(%.2f / %.2f 秒)", now, r.length, now / static_cast<float>(r.fps), r.length / static_cast<float>(r.fps));
	ImGui::SameLine(0, 20);
	ImGui::Checkbox("速く流す", &run.fast);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("描かずに何コマも進める(確認点では必ず描いて比べる)");
}

void EditorApp::drawReplays()
{
	if (!ImGui::Begin(WIN_REPLAYS))
	{
		ImGui::End();
		return;
	}
	refreshReplays();
	std::string reason;
	bool can = canStartReplay(&reason);

	ImGui::BeginDisabled(!can);
	if (ImGui::Button("● 記録")) startRecording();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", can ? "今のシーンを最初から再生し、操作を記録する" : reason.c_str());
	ImGui::SameLine();
	ImGui::BeginDisabled(!can || replayFiles.empty());
	if (ImGui::Button("全部検査"))
	{
		checkAllReplays();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", can ? "全部を速く流し、確認点の画面と状態を基準と比べる(ctest と同じ)" : reason.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("%s", replayDir().generic_string().c_str());

	if (replayRun)
	{
		ImGui::Separator();
		bool update = replayRun->check->getMode() == ReplayCheck::Mode::Update;
		const char* head = replayRun->finished ? (update ? "基準を作り直しました:" : "流し終えました:") : (update ? "基準を作り直しています:" : "流しています:");
		ImGui::Text("%s %s", head, replayRun->name.c_str());
		if (replayRun->finished)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(■ 停止でシーンに戻る。帯を押すとその場面まで流し直す)");
		}
		if (!replayQueue.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(残り %d 本)", static_cast<int>(replayQueue.size()));
		}
		drawTimeStrip();
	}
	ImGui::Separator();

	if (replayFiles.empty() && replaySummaries.empty())
	{
		ImGui::TextDisabled("再現テストはまだありません。● 記録 で操作を記録すると、ここに並びます");
	}
	else if (ImGui::BeginTable("replays", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("長さ");
		ImGui::TableSetupColumn("確認点");
		ImGui::TableSetupColumn("結果");
		ImGui::TableSetupColumn("");
		ImGui::TableHeadersRow();
		for (auto& [name, r] : replayFiles)
		{
			ImGui::PushID(name.c_str());
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Selectable(name.c_str(), replaySelected == name, ImGuiSelectableFlags_AllowOverlap)) replaySelected = name;
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("シーン: %s\n入力の変化: %d 回", r.scene.empty() ? "(project.json の startScene)" : r.scene.c_str(), static_cast<int>(r.events.size()));
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.1f 秒", r.length / static_cast<float>(r.fps));
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%d", static_cast<int>(r.checkpoints.size()));
			ImGui::TableSetColumnIndex(3);
			auto it = replaySummaries.find(name);
			if (it == replaySummaries.end()) ImGui::TextDisabled("未実行");
			else if (it->second.update) ImGui::TextColored(COLOR_OK, "基準を作成");
			else if (it->second.passed) ImGui::TextColored(COLOR_OK, "一致");
			else ImGui::TextColored(COLOR_ERROR, "違う");
			ImGui::TableSetColumnIndex(4);
			ImGui::BeginDisabled(!can);
			if (ImGui::SmallButton("流す")) startReplay(name, ReplayCheck::Mode::Check, false);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", can ? "記録の操作で再生し、確認点で基準と比べる" : reason.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("基準を作り直す")) startReplay(name, ReplayCheck::Mode::Update, true);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", can ? "今の動きを正しいものとして、確認点の画面と状態を保存し直す" : reason.c_str());
			ImGui::EndDisabled();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	//選んだテストの結果(行を押すとコピー)
	auto it = replaySummaries.find(replaySelected);
	if (it != replaySummaries.end() && !it->second.lines.empty())
	{
		ImGui::Separator();
		for (auto& line : it->second.lines)
		{
			bool bad = line.find("DIFFERS") != std::string::npos || line.find("FAILED") != std::string::npos || line.find("stopped") != std::string::npos;
			if (bad) ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ERROR);
			if (ImGui::Selectable(line.c_str())) ImGui::SetClipboardText(line.c_str());
			if (bad) ImGui::PopStyleColor();
		}
	}
	ImGui::End();
}

int EditorApp::run()
{
	if (debug.listCommands)
	{
		printCommands();
		return 0;
	}
	using clock = std::chrono::steady_clock;
	auto lastTime = clock::now();
	auto nextFrame = lastTime;
	const auto frameTime = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / 60));
	int frame = 0;

	while (!quit)
	{
		window.processMessage();
		if (window.closeRequested())
		{
			window.cancelClose();
			request(Pending::Quit);
		}
		frame++;
		auto now = clock::now();
		float dt = std::chrono::duration<float>(now - lastTime).count();
		lastTime = now;
		if (fixedStep && editor->isPlaying()) dt = 1.0f / 60.0f;
		if (debug.fixedDeltaTime > 0) dt = debug.fixedDeltaTime;

		// ---- ImGui の受付(入力をゲームに渡すかの判断に使う)
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		applyScriptedInput(frame);
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		// ---- ゲームの入力: 再生中で、文字入力中でない時だけ。マウスはシーンの画面の中の座標に直す
		input.inputStateUpdate();
		inputState = input.getInputState();
		debug.apply(frame, inputState);
		ImGuiIO& io = ImGui::GetIO();
		if (!editor->isPlaying() || io.WantTextInput)
		{
			std::fill(std::begin(inputState.key), std::end(inputState.key), 0);
			std::fill(std::begin(inputState.deltakey), std::end(inputState.deltakey), 0);
		}
		//シーンの画面の外のクリック(ツールバーなど)はゲームに渡さない(記録にも残さない)
		if (!viewHovered)
		{
			for (InputKey k : { InputKey::MOUSE_LEFT, InputKey::MOUSE_RIGHT, InputKey::MOUSE_MIDDLE, InputKey::MOUSE_4, InputKey::MOUSE_5 })
			{
				inputState.key[static_cast<int>(k)] = 0;
				inputState.deltakey[static_cast<int>(k)] = 0;
			}
		}
		if (viewSize.x > 0)
		{
			inputState.mouse.position = Vec2((io.MousePos.x - viewMin.x) * gameWidth / viewSize.x, (io.MousePos.y - viewMin.y) * gameHeight / viewSize.y);
		}

		if (recorder) dt = 1.0f / 60.0f;
		if (replayRun) dt = replayRun->replay.deltaTime();
		//速く流す時は描かずに何コマも進める(描画の積み残しとイベントはコマごとに捨てる)。確認点のコマで止めて描く
		int steps = replaySteps();
		for (int i = 0; i < steps; i++)
		{
			if (i > 0)
			{
				renderQueue.clear();
				EventBus::getInstance().cleanup();
			}
			bool advance = editor->willAdvance();
			int next = editor->getPlayFrame() + 1;
			if (replayRun && advance) replayRun->player->apply(next, inputState);
			if (recorder && advance)
			{
				recorder->record(next, inputState);
				recordLastFrame = next;
			}
			editor->update(dt);
			if (AudioCommand e; audioRequest.trigger(e) && i == steps - 1) audioQueue.addCommand({ e.audioID, e.volume });
			if (replayRun && advance && replayRun->replay.isCheckpoint(next)) break;
		}
		if (recorder && !editor->isPlaying()) finishRecording();

		//外で変わった assets の JSON を見る(0.5 秒ごと。再生中は止めてから)
		if (frame == 1 || frame % 30 == 0) checkFiles();

		// ---- 画面を組み立てる
		ImGuiID dockspace = ImGui::GetID("EditorDock");
		if (!layoutBuilt)
		{
			if (!ImGui::DockBuilderGetNode(dockspace) || io.IniFilename == nullptr)
				buildDefaultLayout(dockspace);
			layoutBuilt = true;
		}
		if (resetLayout)
		{
			buildDefaultLayout(dockspace);
			resetLayout = false;
		}
		//上下の帯はドックより先に出して、ドックの場所を空けてもらう
		if (editor->isPlaying()) ImGui::PushStyleColor(ImGuiCol_MenuBarBg, PLAY_TINT);
		drawMenuBar();
		if (editor->isPlaying()) ImGui::PopStyleColor();
		drawToolbar();
		drawStatusBar();
		ImGui::DockSpaceOverViewport(dockspace, ImGui::GetMainViewport());
		drawHierarchy();
		drawInspector();
		drawSceneView();
		drawAssets();
		drawSystems();
		if (focusLog)
		{
			ImGui::SetWindowFocus(WIN_LOG);
			focusLog = false;
		}
		drawLog();
		drawReplays();
		if (focusChanges)
		{
			ImGui::SetWindowFocus(WIN_CHANGES);
			focusChanges = false;
		}
		drawChanges();
		drawPopups();
		if (frame == 2 && io.IniFilename == nullptr) ImGui::SetWindowFocus(WIN_REPLAYS); //配置を作り直した時は下の段を再現テストにしておく(コンセプトの柱を目に入る所に)
		drawPalette();
		handleShortcuts();
		for (auto& c : debug.commandsAt(frame))
		{
			auto eq = c.find('=');
			runCommand(c.substr(0, eq), eq == std::string::npos ? "" : c.substr(eq + 1));
		}
		updateCamera();

		// ---- シーンの画面を描き先に描く(止めている時の 3D はエディタカメラから)
		view3D = false;
		if (!editor->isPlaying() && useEditorCamera && (renderQueue.getCamera3D() || !renderQueue.getMeshes().empty()))
		{
			Camera3DView view = renderQueue.getCamera3D().value_or(Camera3DView{ glm::mat4(1.0f), 60, 0.1f, 1000, Vec3(0.45f, 0.6f, 0.8f) });
			view.view = camera.view();
			renderQueue.setCamera3D(view);
			view3D = true;
			viewFov = view.fov;
			viewNear = view.nearClip;
			viewFar = view.farClip;
		}
		drawer.setRenderTarget(gameTarget);
		renderer.render(drawer, renderQueue, editor->world().getCamera());
		afterReplayFrame();
		drawer.setRenderTarget(-1);

		// ---- エディタの画面
		ImGui::Render();
		int w, h;
		glfwGetFramebufferSize(window.handle(), &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(0.1f, 0.1f, 0.12f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		for (auto& path : debug.screenshotsAt(frame))
		{
			if (!drawer.captureScreen(path)) std::cerr << "screenshot failed: " << path << std::endl;
		}
		drawer.present();
		audioPlayer.play(audioBackend, audioQueue);
		EventBus::getInstance().cleanup();

		std::string title = "G.O.M.I Editor - " + (editor->getPath().empty() ? std::string("(新しいシーン)") : editor->getPath()) + (editor->isDirty() ? " *" : "");
		window.setTitle(title);

		if (debug.shouldQuit(frame)) break;
		nextFrame += frameTime;
		now = clock::now();
		if (nextFrame < now - frameTime) nextFrame = now;
		if (nextFrame - now > std::chrono::milliseconds(1)) std::this_thread::sleep_until(nextFrame - std::chrono::milliseconds(1));
		while (clock::now() < nextFrame) std::this_thread::yield();
	}
	return 0;
}

// ---- 保存済みとの差・外で変わったファイル

void EditorApp::checkFiles()
{
	auto result = editor->checkFiles();
	if (result.changed.empty()) return;
	refreshAssets(true);
	std::string others;
	int count = 0;
	for (auto& id : result.changed)
	{
		if (id == editor->getPath()) continue;
		if (count < 3) others += (count ? ", " : "") + id;
		count++;
	}
	if (count > 3) others += " ほか " + std::to_string(count - 3) + " 件";
	if (result.refreshed) log("外で変わったファイルを読み直しました: " + others);
	if (result.reloaded) log("外で変わったシーンを読み直しました: " + editor->getPath() + "(Ctrl+Z で前に戻せる)");
	if (result.sceneConflict)
	{
		fileChangeList = editor->fileChanges();
		openFileConflict = true;
		log(editor->getPath() + " が外で変わりましたが、保存していない変更があるので読み直していません", LogLevel::Warning);
	}
	if (!result.error.empty()) log("外で変わったファイルを読めませんでした(書きかけかもしれません。今の状態のまま): " + result.error, LogLevel::Warning);
	refreshChanges(true);
}

void EditorApp::reloadScene()
{
	std::string error;
	if (editor->reloadFromFile(error)) log("ファイルから読み直しました: " + editor->getPath() + "(Ctrl+Z で前に戻せる)");
	else log("読み直せませんでした: " + error, LogLevel::Error);
	refreshChanges(true);
}

void EditorApp::refreshChanges(bool force)
{
	double now = ImGui::GetTime();
	if (!force && now - changesAt < 0.25) return;
	changesAt = now;
	changes = editor->changesFromSaved();
}

void EditorApp::drawChangeTable(const char* id, const std::vector<SceneChange>& list, const char* beforeLabel, const char* afterLabel, bool canSelect)
{
	using Kind = SceneChange::Kind;
	ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
	if (!ImGui::BeginTable(id, 4, flags)) return;
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthStretch, 1.2f);
	ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthStretch, 1.4f);
	ImGui::TableSetupColumn(beforeLabel, ImGuiTableColumnFlags_WidthStretch, 1.5f);
	ImGui::TableSetupColumn(afterLabel, ImGuiTableColumnFlags_WidthStretch, 1.5f);
	ImGui::TableHeadersRow();
	for (size_t i = 0; i < list.size(); i++)
	{
		const SceneChange& c = list[i];
		ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		std::string entity = c.entity, item, before, after;
		switch (c.kind)
		{
		case Kind::EntityAdded: color = COLOR_OK; item = "(Entity を追加)"; break;
		case Kind::EntityRemoved: color = COLOR_ERROR; item = "(Entity を削除)"; break;
		case Kind::ComponentAdded: color = COLOR_OK; item = "+ " + c.component; break;
		case Kind::ComponentRemoved: color = COLOR_ERROR; item = "- " + c.component; break;
		case Kind::FieldChanged:
			item = c.field.empty() ? c.component : c.component + "." + c.field;
			before = SceneDiff::shortValue(c.before, 48);
			after = SceneDiff::shortValue(c.after, 48);
			break;
		case Kind::SystemsChanged: entity = "(シーン)"; item = "System の並び"; before = SceneDiff::shortValue(c.before, 48); after = SceneDiff::shortValue(c.after, 48); break;
		case Kind::UIChanged: entity = "(シーン)"; item = "UI"; before = SceneDiff::shortValue(c.before, 48); after = SceneDiff::shortValue(c.after, 48); break;
		}
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::PushID(static_cast<int>(i));
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		bool chosen = canSelect && c.index >= 0 && selected == c.index;
		if (ImGui::Selectable(entity.c_str(), chosen, ImGuiSelectableFlags_SpanAllColumns) && canSelect && c.index >= 0) selected = c.index;
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered() && (c.kind == Kind::FieldChanged || c.kind == Kind::SystemsChanged || c.kind == Kind::UIChanged))
		{
			ImGui::BeginTooltip();
			ImGui::TextDisabled("%s", beforeLabel);
			ImGui::TextUnformatted(c.before.dump(2).c_str());
			ImGui::TextDisabled("%s", afterLabel);
			ImGui::TextUnformatted(c.after.dump(2).c_str());
			ImGui::EndTooltip();
		}
		ImGui::PopID();
		ImGui::TableNextColumn();
		ImGui::TextColored(color, "%s", item.c_str());
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", before.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(after.c_str());
	}
	ImGui::EndTable();
}

void EditorApp::drawChanges()
{
	if (!ImGui::Begin(WIN_CHANGES))
	{
		ImGui::End();
		return;
	}
	refreshChanges();
	const std::string& path = editor->getPath();
	if (editor->hasSceneConflict())
	{
		ImGui::TextColored(COLOR_WARNING, "%s が外で書き換えられました(保存していない変更があるので読み直していません)", path.c_str());
		if (ImGui::Button("読み直す")) reloadScene();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("エディタの変更を捨ててファイルを読む(Ctrl+Z で戻せる)");
		ImGui::SameLine();
		if (ImGui::Button("このまま続ける"))
		{
			editor->keepEdits();
			refreshChanges(true);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("保存するとファイルを上書きする");
		ImGui::SameLine();
		if (ImGui::Button("外の変更を見る")) openFileConflict = true;
		ImGui::Separator();
	}
	std::string base = path.empty() ? std::string("新しいシーン(作った時)") : path;
	if (changes.empty()) ImGui::TextDisabled("%s と同じです", base.c_str());
	else ImGui::Text("%s からの変更: %d 件", base.c_str(), static_cast<int>(changes.size()));
	if (editor->isPlaying())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(再生前の状態と比べています)");
	}
	if (!changes.empty()) drawChangeTable("##changes", changes, "保存済み", "今", true);
	ImGui::End();
}

// ---- コマンド

void EditorApp::registerCommands()
{
	auto add = [this](const char* id, const char* label, const char* shortcut, ImGuiKeyChord chord, std::function<bool()> enabled, std::function<void(const std::string&)> run) -> EditorCommand& {
		EditorCommand c;
		c.id = id;
		c.label = label;
		c.shortcut = shortcut;
		c.chord = chord;
		c.enabled = std::move(enabled);
		c.run = std::move(run);
		commands.push_back(std::move(c));
		return commands.back();
	};
	auto stopped = [this] { return !editor->isPlaying(); };
	auto playing = [this] { return editor->isPlaying(); };
	auto hasSelection = [this] { return selected >= 0 && !editor->isPlaying(); };
	auto hasSelectionNow = hasSelection;
	auto lastSelected = [this](int count) { selected = static_cast<int>(editor->entities().size()) - count; };

	// ---- ファイル
	add("scene.new", "新しいシーン", "Ctrl+N", ImGuiMod_Ctrl | ImGuiKey_N, stopped, [this](const std::string&) { request(Pending::NewScene); });
	{
		auto& c = add("scene.open", "シーンを開く", "", 0, stopped, [this](const std::string& a) { request(Pending::OpenScene, a); });
		c.choices = [this] { return assetsIn("scenes", { ".json" }); };
		c.argName = "シーン(Field か scenes/Field.json)";
	}
	add("scene.save", "保存", "Ctrl+S", ImGuiMod_Ctrl | ImGuiKey_S, stopped, [this](const std::string&) { saveScene(); });
	add("scene.saveAs", "名前を付けて保存", "", 0, stopped, [this](const std::string& a) {
		if (a.empty())
		{
			openSaveAs = true;
			saveAsPath = editor->getPath().empty() ? "scenes/NewScene.json" : editor->getPath();
			return;
		}
		std::string error, id = SceneLoader::scenePath(a);
		if (editor->saveAs(id, error))
		{
			log("保存しました: " + id);
			refreshAssets(true);
		}
		else log("保存できませんでした: " + error, LogLevel::Error);
	}).argName = "(省略可)保存先(scenes/X.json)。省略すると名前を聞く";
	add("scene.reload", "ファイルから読み直す", "", 0, [this] { return !editor->isPlaying() && !editor->getPath().empty(); }, [this](const std::string&) { reloadScene(); });
	add("app.quit", "終了", "", 0, nullptr, [this](const std::string&) { request(Pending::Quit); });

	// ---- 編集
	add("edit.undo", "元に戻す", "Ctrl+Z", ImGuiMod_Ctrl | ImGuiKey_Z, [this] { return editor->canUndo(); }, [this](const std::string&) { editor->undo(); });
	add("edit.redo", "やり直す", "Ctrl+Y", ImGuiMod_Ctrl | ImGuiKey_Y, [this] { return editor->canRedo(); }, [this](const std::string&) { editor->redo(); });
	add("entity.create", "空の Entity を作る", "", 0, stopped, [this, lastSelected](const std::string& a) {
		editor->createEntity(a.empty() ? "Entity" : a);
		lastSelected(1);
	}).argName = "(省略可)名前";
	{
		auto& c = add("entity.placePrefab", "プレハブを置く", "", 0, stopped, [this, lastSelected](const std::string& a) {
			auto made = editor->createFromPrefab(a);
			if (made.empty()) log("プレハブを置けませんでした: " + a, LogLevel::Error);
			else lastSelected(static_cast<int>(made.size()));
		});
		c.choices = [this] { return assetsIn("prefabs", { ".json" }); };
		c.argName = "プレハブ(prefabs/X.json)";
	}
	add("entity.duplicate", "複製", "Ctrl+D", ImGuiMod_Ctrl | ImGuiKey_D, hasSelection, [this, lastSelected](const std::string&) {
		editor->duplicate(selectedEntity());
		lastSelected(1);
	});
	add("entity.rename", "名前を変える", "F2", ImGuiKey_F2, hasSelection, [this](const std::string& a) {
		if (a.empty()) startRename(selected);
		else editor->rename(selectedEntity(), a);
	}).argName = "(省略可)新しい名前。省略するとヒエラルキーで直す";
	add("hierarchy.search", "ヒエラルキーを絞り込む", "Ctrl+F", ImGuiMod_Ctrl | ImGuiKey_F, nullptr, [this](const std::string& a) {
		ImGui::SetWindowFocus(WIN_HIERARCHY);
		if (a.empty()) focusFilter = true;
		else hierarchyFilter = a;
	}).argName = "(省略可)絞り込む文字";
	add("prefab.saveSelection", "選んだ物をプレハブにする", "", 0, hasSelection, [this](const std::string& a) {
		if (!a.empty())
		{
			savePrefab(a);
			return;
		}
		std::string name = editor->displayName(selectedEntity());
		for (auto& ch : name) if (ch == ' ' || ch == '#' || ch == '[' || ch == ']') ch = '_';
		prefabSavePath = "prefabs/" + name + ".json";
		openPrefabSave = true;
	}).argName = "(省略可)保存先(prefabs/X.json)。省略すると名前を聞く";
	add("prefab.edit", "選んだ物のプレハブを開いて直す", "", 0, [this, hasSelectionNow] { return hasSelectionNow() && editor->world().getPrefabLink(selectedEntity()) != nullptr; }, [this](const std::string&) {
		editPrefab(editor->world().getPrefabLink(selectedEntity())->prefab);
	});
	{
		auto& c = add("prefab.open", "プレハブを開いて直す", "", 0, stopped, [this](const std::string& a) { editPrefab(a); });
		c.choices = [this] { return assetsIn("prefabs", { ".json" }); };
		c.argName = "プレハブ(prefabs/X.json)";
	}
	add("prefab.back", "プレハブからシーンに戻る", "", 0, [this] { return editor->isPrefabMode() && !returnScene.empty(); }, [this](const std::string&) {
		request(Pending::OpenScene, returnScene);
	});
	add("entity.copy", "JSON で写す", "Ctrl+C", ImGuiMod_Ctrl | ImGuiKey_C, [this] { return selected >= 0; }, [this](const std::string&) {
		Json item = editor->copyEntity(selectedEntity());
		ImGui::SetClipboardText(item.dump(2).c_str());
		log("写しました(シーンの entities の1項目の形): " + editor->displayName(selectedEntity()));
	});
	add("entity.paste", "JSON を貼る", "Ctrl+V", ImGuiMod_Ctrl | ImGuiKey_V, stopped, [this](const std::string& a) {
		std::string text = a.empty() ? (ImGui::GetClipboardText() ? ImGui::GetClipboardText() : "") : a;
		Json items;
		try
		{
			items = Json::parse(text);
		}
		catch (const std::exception& e)
		{
			log(std::string("貼れませんでした(JSON ではない): ") + e.what(), LogLevel::Error);
			return;
		}
		int count = 0;
		std::string error;
		if (editor->pasteEntities(items, count, error))
		{
			selected = static_cast<int>(editor->entities().size()) - count;
			log("貼りました: " + std::to_string(count) + " 個");
		}
		else log("貼れませんでした: " + error, LogLevel::Error);
	}).argName = "(省略可)JSON。省略するとクリップボードから";
	add("entity.delete", "削除", "Delete", ImGuiKey_Delete, hasSelection, [this](const std::string&) {
		editor->destroy(selectedEntity());
		selected = -1;
	}).shortcutWhen = [this] { return selected >= 0 && !editor->isPlaying() && !ImGui::IsAnyItemActive(); };
	{
		auto& c = add("entity.select", "選ぶ", "", 0, nullptr, [this](const std::string& a) {
			auto list = editor->entities();
			auto names = editor->displayNames();
			for (int i = 0; i < static_cast<int>(list.size()); i++)
			{
				if (names[i] == a)
				{
					selected = i;
					return;
				}
			}
			try
			{
				int i = std::stoi(a);
				if (i >= 0 && i < static_cast<int>(list.size()))
				{
					selected = i;
					return;
				}
			}
			catch (...)
			{
			}
			log("選べませんでした(名前か並び順の番号): " + a, LogLevel::Warning);
		});
		c.choices = [this] {
			return editor->displayNames();
		};
		c.argName = "Entity の名前か並び順の番号";
	}
	add("view.focusSelected", "選んだ物にエディタカメラを向ける", "F", ImGuiKey_F, [this] { return selected >= 0; }, [this](const std::string&) {
		Json p = editor->getField(selectedEntity(), "Transform", "position");
		if (p.is_array()) camera.target = glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
	}).shortcutWhen = [this] { return selected >= 0 && viewHovered; };

	// ---- 再生
	add("play.toggle", "再生 / 停止", "F5", ImGuiKey_F5, nullptr, [this](const std::string&) { togglePlay(); });
	add("play.pause", "一時停止 / 再開", "F6", ImGuiKey_F6, playing, [this](const std::string&) { editor->setPaused(!editor->isPaused()); });
	add("play.step", "1コマ進める", "F7", ImGuiKey_F7, playing, [this](const std::string&) { editor->stepFrame(); });
	add("play.fixedStep", "Δt 1/60 固定を切り替える", "", 0, nullptr, [this](const std::string&) { fixedStep = !fixedStep; });
	add("replay.record", "操作を記録する", "", 0, [this] { return canStartReplay(); }, [this](const std::string&) { startRecording(); });
	add("replay.checkAll", "再現テストを全部検査する", "", 0, [this] { return !replayFiles.empty() && canStartReplay(); }, [this](const std::string&) { checkAllReplays(); });
	{
		auto names = [this] {
			std::vector<std::string> list;
			for (auto& [name, r] : replayFiles) list.push_back(name);
			return list;
		};
		auto& run = add("replay.run", "再現テストを流す", "", 0, [this] { return canStartReplay(); }, [this](const std::string& a) { startReplay(a, ReplayCheck::Mode::Check, false); });
		run.choices = names;
		run.argName = "再現テストの名前";
		auto& update = add("replay.update", "再現テストの基準を作り直す", "", 0, [this] { return canStartReplay(); }, [this](const std::string& a) { startReplay(a, ReplayCheck::Mode::Update, true); });
		update.choices = names;
		update.argName = "再現テストの名前";
	}

	add("replay.seek", "再現テストの場面へ進む・戻る", "", 0, [this] { return replayRun.has_value(); }, [this](const std::string& a) {
		try
		{
			seekReplay(std::stoi(a));
		}
		catch (...)
		{
			log("フレームの番号を渡してください: " + a, LogLevel::Error);
		}
	}).argName = "フレーム番号";

	// ---- 表示
	add("gizmo.move", "ギズモ: 移動", "W", ImGuiKey_W, nullptr, [this](const std::string&) { gizmoOperation = 0; });
	add("gizmo.rotate", "ギズモ: 回転", "E", ImGuiKey_E, nullptr, [this](const std::string&) { gizmoOperation = 1; });
	add("gizmo.scale", "ギズモ: 拡大", "R", ImGuiKey_R, nullptr, [this](const std::string&) { gizmoOperation = 2; });
	add("view.editorCamera", "エディタカメラを切り替える", "", 0, nullptr, [this](const std::string&) { useEditorCamera = !useEditorCamera; });
	add("palette.open", "コマンドパレット", "Ctrl+Shift+P", ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, nullptr, [this](const std::string&) { openPalette = true; });
	const std::pair<const char*, const char*> windows[] = {
		{ "window.hierarchy", WIN_HIERARCHY }, { "window.inspector", WIN_INSPECTOR }, { "window.scene", WIN_SCENE },
		{ "window.replays", WIN_REPLAYS }, { "window.changes", WIN_CHANGES }, { "window.assets", WIN_ASSETS },
		{ "window.systems", WIN_SYSTEMS }, { "window.log", WIN_LOG },
	};
	for (auto& [id, name] : windows)
	{
		std::string label = std::string(name) + "を開く";
		std::string window = name;
		add(id, label.c_str(), "", 0, nullptr, [this, window](const std::string&) {
			if (window == WIN_LOG) focusLog = true;
			else if (window == WIN_CHANGES) focusChanges = true;
			else ImGui::SetWindowFocus(window.c_str());
		});
	}
	add("layout.reset", "窓の配置を初めに戻す", "", 0, nullptr, [this](const std::string&) { resetLayout = true; });
}

const EditorCommand* EditorApp::findCommand(const std::string& id) const
{
	for (auto& c : commands)
	{
		if (c.id == id) return &c;
	}
	return nullptr;
}

bool EditorApp::runCommand(const std::string& id, const std::string& arg)
{
	const EditorCommand* c = findCommand(id);
	if (!c)
	{
		log("知らないコマンド: " + id + "(--list-commands で一覧)", LogLevel::Error);
		return false;
	}
	if (!c->isEnabled())
	{
		log("今は使えないコマンド: " + id, LogLevel::Warning);
		return false;
	}
	if (c->choices && arg.empty())
	{
		log("コマンド " + id + " には引数が要ります: " + c->argName, LogLevel::Error);
		return false;
	}
	std::string key = id + "\n" + arg;
	recentCommands.erase(std::remove(recentCommands.begin(), recentCommands.end(), key), recentCommands.end());
	recentCommands.insert(recentCommands.begin(), key);
	if (recentCommands.size() > 10) recentCommands.pop_back();
	c->run(arg);
	return true;
}

void EditorApp::menuItem(const std::string& id)
{
	const EditorCommand* c = findCommand(id);
	if (!c) return;
	if (ImGui::MenuItem(c->label.c_str(), c->shortcut.empty() ? nullptr : c->shortcut.c_str(), false, c->isEnabled())) runCommand(id);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", c->id.c_str());
}

void EditorApp::printCommands() const
{
	//ID・ショートカット・名前・引数をタブ区切りで(AI やスクリプトが読む)
	for (auto& c : commands)
		std::cout << c.id << "\t" << c.shortcut << "\t" << c.label << "\t" << c.argName << "\n";
	std::cout.flush();
}

void EditorApp::startRename(int index)
{
	auto list = editor->entities();
	if (index < 0 || index >= static_cast<int>(list.size()) || editor->isPlaying()) return;
	selected = index;
	renaming = index;
	renameFocus = true;
	Json name = editor->getField(list[index], "Name", "name");
	renameText = name.is_string() && !name.get<std::string>().empty() ? name.get<std::string>() : editor->displayName(list[index]);
	ImGui::SetWindowFocus(WIN_HIERARCHY);
}

void EditorApp::editPrefab(const std::string& prefab)
{
	if (!editor->isPrefabMode() && !editor->getPath().empty()) returnScene = editor->getPath();
	request(Pending::OpenScene, prefab);
}

void EditorApp::savePrefab(const std::string& prefab)
{
	std::string error, id = prefab;
	if (id.rfind("prefabs/", 0) != 0) id = "prefabs/" + id;
	if (id.size() < 5 || id.compare(id.size() - 5, 5, ".json") != 0) id += ".json";
	if (editor->saveAsPrefab(selectedEntity(), id, error))
	{
		log("プレハブにしました: " + id + "(シーンを保存すると、この配置が prefab で書かれる)");
		refreshAssets(true);
	}
	else log("プレハブにできませんでした: " + error, LogLevel::Error);
}

void EditorApp::checkAllReplays()
{
	if (replayFiles.empty()) return;
	replayQueue.clear();
	for (auto& [name, r] : replayFiles) replayQueue.push_back(name);
	std::string first = replayQueue.front();
	replayQueue.erase(replayQueue.begin());
	startReplay(first, ReplayCheck::Mode::Check, true);
}

void EditorApp::drawPalette()
{
	const char* POPUP = "##palette";
	if (openPalette)
	{
		ImGui::OpenPopup(POPUP);
		openPalette = false;
		paletteQuery.clear();
		paletteCursor = 0;
	}
	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x * 0.5f, vp->WorkPos.y + 50), ImGuiCond_Always, ImVec2(0.5f, 0));
	ImGui::SetNextWindowSize(ImVec2(std::min(680.0f, vp->WorkSize.x - 40), 0));
	if (!ImGui::BeginPopup(POPUP)) return;

	if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
	ImGui::SetNextItemWidth(-1);
	bool enter = ImGui::InputTextWithHint("##query", "コマンドを名前・ID で探す(↑↓ で選び Enter で実行、Esc で閉じる)", &paletteQuery, ImGuiInputTextFlags_EnterReturnsTrue);
	if (ImGui::IsItemEdited()) paletteCursor = 0;
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	//候補: 引数が要るコマンドは候補ごとに1行。最近使った物を先に
	struct Entry
	{
		const EditorCommand* command;
		std::string arg;
		std::string text;
		int order;
	};
	auto lower = [](std::string s) {
		for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		return s;
	};
	std::vector<std::string> words;
	{
		std::string q = lower(paletteQuery);
		size_t pos = 0;
		while (pos < q.size())
		{
			size_t sp = q.find(' ', pos);
			if (sp == std::string::npos) sp = q.size();
			if (sp > pos) words.push_back(q.substr(pos, sp - pos));
			pos = sp + 1;
		}
	}
	std::vector<Entry> entries;
	int order = 0;
	auto consider = [&](const EditorCommand& c, const std::string& arg) {
		std::string text = c.label + (arg.empty() ? "" : ": " + arg);
		std::string hay = lower(text + " " + c.id + " " + c.shortcut);
		for (auto& w : words)
		{
			if (hay.find(w) == std::string::npos) return;
		}
		auto recent = std::find(recentCommands.begin(), recentCommands.end(), c.id + "\n" + arg);
		int rank = recent == recentCommands.end() ? 1000 + order : static_cast<int>(recent - recentCommands.begin());
		entries.push_back({ &c, arg, text, rank });
		order++;
	};
	for (auto& c : commands)
	{
		if (c.id == "palette.open") continue;
		if (c.choices)
		{
			for (auto& arg : c.choices()) consider(c, arg);
		}
		else consider(c, "");
	}
	std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.order < b.order; });

	if (!entries.empty())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) paletteCursor = std::min(paletteCursor + 1, static_cast<int>(entries.size()) - 1);
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) paletteCursor = std::max(paletteCursor - 1, 0);
	}
	paletteCursor = std::clamp(paletteCursor, 0, std::max(0, static_cast<int>(entries.size()) - 1));

	const Entry* chosen = nullptr;
	if (enter && !entries.empty() && entries[paletteCursor].command->isEnabled()) chosen = &entries[paletteCursor];
	float rowH = ImGui::GetTextLineHeightWithSpacing();
	ImGui::BeginChild("##list", ImVec2(0, rowH * std::clamp(static_cast<float>(entries.size()), 1.0f, 14.0f) + 8));
	if (entries.empty()) ImGui::TextDisabled("見つかりません");
	for (int i = 0; i < static_cast<int>(entries.size()); i++)
	{
		const Entry& e = entries[i];
		bool can = e.command->isEnabled();
		ImGui::PushID(i);
		ImGui::BeginDisabled(!can);
		if (ImGui::Selectable(e.text.c_str(), i == paletteCursor) && can) chosen = &e;
		ImGui::EndDisabled();
		if (i == paletteCursor && (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow))) ImGui::SetScrollHereY();
		//右に ID とショートカット
		std::string right = e.command->id + (e.command->shortcut.empty() ? "" : "   " + e.command->shortcut);
		float w = ImGui::CalcTextSize(right.c_str()).x;
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - w);
		ImGui::TextDisabled("%s", right.c_str());
		ImGui::PopID();
	}
	ImGui::EndChild();
	if (chosen)
	{
		std::string id = chosen->command->id, arg = chosen->arg;
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		runCommand(id, arg);
		return;
	}
	ImGui::EndPopup();
}
