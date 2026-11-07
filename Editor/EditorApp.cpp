#include "EditorApp.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "SettingManager.h"
#include "ResourceManager.h"

#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include <ImGuizmo.h>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
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
	const char* ASSET_PAYLOAD = "GOMI_ASSET";

	bool endsWith(const std::string& s, const char* ext)
	{
		std::string e = ext;
		return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0;
	}
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

	editor = std::make_unique<SceneEditor>(inputState, renderQueue, audioQueue);
	if (!debug.startWorld.empty())
	{
		std::string error;
		if (!editor->open(debug.startWorld, error)) log("開けませんでした: " + error);
	}
	selected = debug.select;
	refreshAssets(true);
}

EditorApp::~EditorApp()
{
	editor.reset();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void EditorApp::log(const std::string& message)
{
	logLines.push_back(message);
	if (logLines.size() > 200) logLines.erase(logLines.begin());
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
		else log("開けませんでした: " + error);
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
	else log("保存できませんでした: " + error);
}

// ---- 画面

void EditorApp::drawMenuBar()
{
	if (!ImGui::BeginMainMenuBar()) return;
	bool playing = editor->isPlaying();
	if (ImGui::BeginMenu("ファイル"))
	{
		if (ImGui::MenuItem("新しいシーン", "Ctrl+N", false, !playing)) request(Pending::NewScene);
		if (ImGui::BeginMenu("開く", !playing))
		{
			for (auto& s : assetsIn("scenes", { ".json" }))
			{
				if (ImGui::MenuItem(s.c_str())) request(Pending::OpenScene, s);
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("保存", "Ctrl+S", false, !playing)) saveScene();
		if (ImGui::MenuItem("名前を付けて保存...", nullptr, false, !playing))
		{
			openSaveAs = true;
			saveAsPath = editor->getPath().empty() ? "scenes/NewScene.json" : editor->getPath();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("終了")) request(Pending::Quit);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("編集"))
	{
		if (ImGui::MenuItem("元に戻す", "Ctrl+Z", false, editor->canUndo())) editor->undo();
		if (ImGui::MenuItem("やり直す", "Ctrl+Y", false, editor->canRedo())) editor->redo();
		ImGui::Separator();
		bool has = selected >= 0;
		if (ImGui::MenuItem("複製", "Ctrl+D", false, has && !playing))
		{
			editor->duplicate(selectedEntity());
			selected = static_cast<int>(editor->entities().size()) - 1;
		}
		if (ImGui::MenuItem("削除", "Delete", false, has && !playing))
		{
			editor->destroy(selectedEntity());
			selected = -1;
		}
		ImGui::EndMenu();
	}

	//再生・停止とファイル名
	ImGui::SameLine(0, 30);
	if (!playing)
	{
		if (ImGui::Button("▶ 再生")) editor->play();
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1));
		if (ImGui::Button("■ 停止")) editor->stop();
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "再生中(止めると元に戻ります)");
	}
	ImGui::SameLine(0, 30);
	std::string name = editor->getPath().empty() ? "(新しいシーン)" : editor->getPath();
	ImGui::TextDisabled("%s%s", name.c_str(), editor->isDirty() ? " *" : "");
	ImGui::EndMainMenuBar();
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
				if (made.empty()) log("プレハブを置けませんでした: " + p);
				else selected = static_cast<int>(editor->entities().size()) - static_cast<int>(made.size());
			}
		}
		ImGui::EndPopup();
	}
	ImGui::EndDisabled();
	ImGui::Separator();

	ImGui::BeginChild("list");
	auto list = editor->entities();
	for (int i = 0; i < static_cast<int>(list.size()); i++)
	{
		ImGui::PushID(static_cast<int>(list[i].id));
		std::string label = editor->displayName(list[i]);
		if (editor->world().getPrefabLink(list[i])) label = "◆ " + label;
		if (ImGui::Selectable(label.c_str(), selected == i)) selected = i;
		if (!playing && ImGui::BeginPopupContextItem())
		{
			selected = i;
			if (ImGui::MenuItem("複製"))
			{
				editor->duplicate(list[i]);
				selected = static_cast<int>(editor->entities().size()) - 1;
			}
			if (ImGui::MenuItem("削除"))
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

bool EditorApp::drawField(Entity e, const ComponentInfo& info, const FieldInfo& f)
{
	Json v = editor->getField(e, info.name, f.name);
	const char* label = f.name.c_str();
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
		changed = ImGui::DragFloat2(label, a, 0.05f, 0, 0, "%.3f");
		startEdit();
		next = Json::array({ a[0], a[1] });
		break;
	}
	case FieldKind::Vec3:
	{
		float a[3] = { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
		float speed = f.name == "rotation" ? 0.5f : 0.05f;
		changed = ImGui::DragFloat3(label, a, speed, 0, 0, "%.3f");
		startEdit();
		next = Json::array({ a[0], a[1], a[2] });
		break;
	}
	case FieldKind::String:
	{
		std::string s = v.get<std::string>();
		bool isAsset = f.name == "path" || f.name == "asset" || f.name == "texture" || f.name == "mesh";
		if (isAsset) ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 30);
		changed = ImGui::InputText(isAsset ? "##value" : label, &s);
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
			ImGui::SameLine();
			ImGui::TextUnformatted(label);
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
		ImGui::TextUnformatted(label);
		Json names = v;
		for (auto& n : f.enumNames)
		{
			if (n == "NONE") continue;
			bool on = std::find(v.begin(), v.end(), Json(n)) != v.end();
			ImGui::SameLine();
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
	case FieldKind::Other:
	{
		//型の決まった欄が無いもの(キー割り当ての表など)は JSON のまま直す
		std::string text = v.dump(1);
		ImGui::InputTextMultiline(label, &text, ImVec2(0, ImGui::GetTextLineHeight() * 6));
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
				log(info.name + "." + f.name + ": JSON の書き方が違います: " + ex.what());
			}
		}
		break;
	}
	}

	if (changed)
	{
		std::string error;
		if (!editor->setField(e, info.name, f.name, next, &error)) log(info.name + "." + f.name + ": " + error);
	}
	return changed;
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
	std::string toRemove;
	for (auto& info : ComponentRegistry::getInstance().all())
	{
		if (!info.has(cm, e)) continue;
		ImGui::PushID(info.name.c_str());
		bool open = true;
		if (ImGui::CollapsingHeader(info.name.c_str(), &open, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(-110);
			for (auto& f : info.fields)
			{
				ImGui::PushID(f.name.c_str());
				drawField(e, info, f);
				ImGui::PopID();
				if (!info.has(cm, e)) break; //編集でシーンが作り直された
			}
			ImGui::PopItemWidth();
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
	ImGui::SetCursorPos(ImVec2(8, ImGui::GetCursorPosY() + 4));
	ImGui::RadioButton("移動 (W)", &gizmoOperation, 0);
	ImGui::SameLine();
	ImGui::RadioButton("回転 (E)", &gizmoOperation, 1);
	ImGui::SameLine();
	ImGui::RadioButton("拡大 (R)", &gizmoOperation, 2);
	ImGui::SameLine(0, 20);
	ImGui::Checkbox("エディタカメラ", &useEditorCamera);
	ImGui::SameLine();
	ImGui::TextDisabled("(3D: 右ドラッグで回す / 中ドラッグで動かす / ホイール / F で選んだ物へ。左クリックで選ぶ)");

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
	drawGizmo();
	//ギズモを触っていない時の左クリックで、そこにある物を選ぶ
	if (viewHovered && !editor->isPlaying() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
	{
		pickAt(ImGui::GetIO().MousePos);
	}
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
		Vec2 cameraPos = editor->world().getCamera().position;
		std::pair<int, int> top{ -1000000, -1000000 };
		for (int i = 0; i < static_cast<int>(list.size()); i++)
		{
			Entity e = list[i];
			if (!cm.has<RendererComponent>(e) || !cm.has<TransformComponent>(e)) continue;
			auto& r = cm.getComponentArray<RendererComponent>().getComponent(e);
			auto& t = cm.getComponentArray<TransformComponent>().getComponent(e);
			Vec2 size = AssetDatabase::getInstance().imageSize(r.path);
			Vec2 min(t.position.x, t.position.y);
			if (r.useCamera) min -= cameraPos;
			Vec2 max = min + Vec2(size.x * t.scale.x, size.y * t.scale.y);
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
	for (auto& l : logLines) ImGui::TextUnformatted(l.c_str());
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
			else log("保存できませんでした: " + error);
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

	ImGui::DockBuilderDockWindow(WIN_SCENE, center);
	ImGui::DockBuilderFinish(dockspace);
}

void EditorApp::handleShortcuts()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput || editor->isPlaying()) return;
	bool ctrl = io.KeyCtrl;
	if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) editor->undo();
	if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) editor->redo();
	if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) saveScene();
	if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) request(Pending::NewScene);
	if (selected >= 0 && ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
	{
		editor->duplicate(selectedEntity());
		selected = static_cast<int>(editor->entities().size()) - 1;
	}
	if (selected >= 0 && ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !ImGui::IsAnyItemActive())
	{
		editor->destroy(selectedEntity());
		selected = -1;
	}
	if (viewHovered && !io.KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W, false)) gizmoOperation = 0;
		if (ImGui::IsKeyPressed(ImGuiKey_E, false)) gizmoOperation = 1;
		if (ImGui::IsKeyPressed(ImGuiKey_R, false)) gizmoOperation = 2;
	}
	//選んだ物にエディタカメラを向ける
	if (selected >= 0 && viewHovered && ImGui::IsKeyPressed(ImGuiKey_F, false))
	{
		Json p = editor->getField(selectedEntity(), "Transform", "position");
		if (p.is_array()) camera.target = glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
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
		else io.AddInputCharactersUTF8(text.c_str());
	}
}

int EditorApp::run()
{
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
		if (viewSize.x > 0)
		{
			inputState.mouse.position = Vec2((io.MousePos.x - viewMin.x) * gameWidth / viewSize.x, (io.MousePos.y - viewMin.y) * gameHeight / viewSize.y);
		}

		editor->update(dt);
		if (AudioCommand e; audioRequest.trigger(e)) audioQueue.addCommand({ e.audioID, e.volume });

		// ---- 画面を組み立てる
		ImGuiID dockspace = ImGui::GetID("EditorDock");
		if (!layoutBuilt)
		{
			if (!ImGui::DockBuilderGetNode(dockspace) || io.IniFilename == nullptr)
				buildDefaultLayout(dockspace);
			layoutBuilt = true;
		}
		ImGui::DockSpaceOverViewport(dockspace, ImGui::GetMainViewport());
		drawMenuBar();
		drawHierarchy();
		drawInspector();
		drawSceneView();
		drawAssets();
		drawSystems();
		drawLog();
		drawPopups();
		if (frame == 2 && io.IniFilename == nullptr) ImGui::SetWindowFocus(WIN_ASSETS); //配置を作り直した時は下の段をアセットにしておく
		handleShortcuts();
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
