// ランチャーの画面(ImGui)。中身は全部 LauncherCore で、ここは並べて呼ぶだけ。
// 時間のかかること(gh を見る・落とす・展開する)は別スレッドで1つずつ行い、進み具合を下に出す
#include "LauncherCore.h"
#include "LauncherOptions.h"
#include <windows.h>
#include <shobjidl.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using namespace launcher;

namespace
{
	const ImVec4 COLOR_WARN = ImVec4(1.0f, 0.7f, 0.3f, 1);
	const ImVec4 COLOR_ERROR = ImVec4(1.0f, 0.45f, 0.45f, 1);
	const ImVec4 COLOR_OK = ImVec4(0.55f, 0.9f, 0.55f, 1);
	const ImVec4 COLOR_DIM = ImVec4(0.6f, 0.6f, 0.6f, 1);

	// エクスプローラーのフォルダ・ファイルの選択。キャンセルなら空
	std::string pickPath(bool folder, const std::string& start)
	{
		std::string result;
		IFileOpenDialog* dialog = nullptr;
		if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return result;
		DWORD flags = 0;
		dialog->GetOptions(&flags);
		dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | (folder ? FOS_PICKFOLDERS : 0));
		if (!folder)
		{
			COMDLG_FILTERSPEC filter[] = { { L"G.O.M.I の配布物 (*.zip)", L"*.zip" } };
			dialog->SetFileTypes(1, filter);
		}
		if (!start.empty())
		{
			IShellItem* item = nullptr;
			if (SUCCEEDED(SHCreateItemFromParsingName(u8path(start).wstring().c_str(), nullptr, IID_PPV_ARGS(&item))))
			{
				dialog->SetFolder(item);
				item->Release();
			}
		}
		if (SUCCEEDED(dialog->Show(nullptr)))
		{
			IShellItem* item = nullptr;
			if (SUCCEEDED(dialog->GetResult(&item)))
			{
				PWSTR path = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
				{
					result = u8str(std::filesystem::path(path));
					CoTaskMemFree(path);
				}
				item->Release();
			}
		}
		dialog->Release();
		return result;
	}

	bool saveScreenshot(GLFWwindow* window, const std::string& path)
	{
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 4);
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		stbi_flip_vertically_on_write(1);
		return stbi_write_png(path.c_str(), w, h, 4, pixels.data(), w * 4) != 0;
	}

	class LauncherApp
	{
	public:
		LauncherApp(LauncherCore& core, const LauncherOptions& options) : core(core), options(options)
		{
			if (options.tab == "versions") tab = Tab::Versions;
			openNewOnStart = options.tab == "new";
			refreshInstalled();
			vscode = LauncherCore::findVSCode();
			checkGhAsync();
		}

		~LauncherApp()
		{
			if (worker.joinable()) worker.join();
		}

		void draw()
		{
			finishJob();
			const ImGuiViewport* vp = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(vp->WorkPos);
			ImGui::SetNextWindowSize(vp->WorkSize);
			ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

			float statusHeight = ImGui::GetFrameHeightWithSpacing() + 4;
			ImGui::BeginChild("##side", ImVec2(170, -statusHeight), ImGuiChildFlags_Borders);
			ImGui::TextDisabled("G.O.M.I");
			ImGui::Separator();
			if (ImGui::Selectable("プロジェクト", tab == Tab::Projects)) tab = Tab::Projects;
			if (ImGui::Selectable("バージョン", tab == Tab::Versions)) tab = Tab::Versions;
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("##main", ImVec2(0, -statusHeight));
			if (tab == Tab::Projects) drawProjects();
			else drawVersions();
			ImGui::EndChild();

			drawStatus();
			drawNewProject();
			drawMissingEngine();
			drawSettings();
			drawDuplicate();
			drawTrash();
			drawUninstall();
			ImGui::End();
		}

		bool busy() const { return jobRunning.load(); }

	private:
		enum class Tab { Projects, Versions };
		LauncherCore& core;
		const LauncherOptions& options;
		Tab tab = Tab::Projects;
		std::vector<std::string> installed;

		// ---- 下の欄の知らせ
		std::string statusText;
		ImVec4 statusColor = COLOR_DIM;

		// ---- 別スレッドの仕事(1つずつ)
		std::thread worker;
		std::atomic<bool> jobRunning{ false };
		std::atomic<bool> jobFinished{ false };
		std::mutex jobMutex;
		std::string jobProgress;
		bool jobOk = false;
		std::string jobMessage;
		std::function<void(bool)> jobThen; // 終わった時に画面のスレッドで呼ぶ

		// ---- gh と Releases
		bool ghChecked = false;
		GhStatus gh;
		std::vector<Release> releases;
		std::string releasesError;

		// ---- 新しいプロジェクト
		bool openNewOnStart = false; // 次のコマで「新しいプロジェクト」を開く
		std::string newName = "NewGame";
		std::string newParent;
		std::string newVersion;
		std::string newTemplate;
		std::string newError;

		// ---- 入っていないバージョンのプロジェクトを開こうとした
		std::string missingProject;
		std::string missingVersion;
		std::string otherVersion;
		bool missingOpenRequest = false;

		// ---- 一覧の行の編集(開くのはそれぞれの draw の中、BeginPopupModal の直前)
		std::filesystem::path vscode; // 見つからなければ空
		std::string editProject;      // 編集しているプロジェクトのフォルダ
		bool settingsRequest = false, duplicateRequest = false, trashRequest = false;
		std::string setName, setEngine, setScene, setPauseUI, setError;
		std::string dupName, dupError;
		std::string trashError;
		// ---- バージョンを消す
		bool uninstallRequest = false;
		std::string uninstallVersion;
		std::vector<std::string> uninstallUsers;

		void setStatus(const std::string& text, ImVec4 color)
		{
			statusText = text;
			statusColor = color;
		}

		void refreshInstalled()
		{
			installed = core.installedVersions();
		}

		// work は別スレッドで動く。progress で途中の文を出し、message に結果の文を入れて成否を返す
		void startJob(std::function<bool(std::string& message, const LauncherCore::Progress& progress)> work, std::function<void(bool)> then = {})
		{
			if (jobRunning) return;
			if (worker.joinable()) worker.join();
			jobRunning = true;
			jobFinished = false;
			jobThen = std::move(then);
			{
				std::lock_guard lock(jobMutex);
				jobProgress = "…";
			}
			worker = std::thread([this, work]() {
				std::string message;
				bool ok = work(message, [this](const std::string& p) {
					std::lock_guard lock(jobMutex);
					jobProgress = p;
				});
				{
					std::lock_guard lock(jobMutex);
					jobOk = ok;
					jobMessage = message;
				}
				jobFinished = true;
			});
		}

		void finishJob()
		{
			if (!jobFinished) return;
			if (worker.joinable()) worker.join();
			jobFinished = false;
			jobRunning = false;
			bool ok;
			std::string message;
			{
				std::lock_guard lock(jobMutex);
				ok = jobOk;
				message = jobMessage;
			}
			refreshInstalled();
			if (!message.empty()) setStatus(message, ok ? COLOR_OK : COLOR_ERROR);
			if (jobThen)
			{
				auto then = std::move(jobThen);
				jobThen = {};
				then(ok);
			}
		}

		void checkGhAsync()
		{
			ghChecked = false;
			auto result = std::make_shared<std::pair<GhStatus, std::vector<Release>>>();
			auto error = std::make_shared<std::string>();
			startJob([this, result, error](std::string&, const LauncherCore::Progress& progress) {
				progress("gh を確かめています…");
				result->first = core.checkGh();
				if (result->first.ok())
				{
					progress("Releases の一覧を取っています…");
					core.listReleases(result->second, *error);
				}
				return true;
			}, [this, result, error](bool) {
				gh = result->first;
				releases = result->second;
				releasesError = *error;
				ghChecked = true;
			});
		}

		// ---------------------------------------------------- プロジェクト
		void drawProjects()
		{
			ImGui::BeginDisabled(installed.empty());
			if (ImGui::Button("新しいプロジェクト")) openNewOnStart = true; //開くのは drawNewProject(ポップアップは開く所と同じ ID の場所で開く)
			ImGui::EndDisabled();
			if (installed.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("先に「バージョン」でエンジンを入れてください");
			ImGui::SameLine();
			if (ImGui::Button("既存のプロジェクトを追加…"))
			{
				std::string dir = pickPath(true, core.projectsDir());
				std::string error;
				if (!dir.empty())
				{
					if (core.addProject(u8path(dir), error)) setStatus("一覧に足しました: " + dir, COLOR_OK);
					else setStatus(error, COLOR_ERROR);
				}
			}
			ImGui::Spacing();

			auto list = core.projects();
			std::stable_sort(list.begin(), list.end(), [](auto& a, auto& b) { return a.lastOpened > b.lastOpened; });
			if (list.empty())
			{
				ImGui::TextDisabled("プロジェクトはまだありません。「新しいプロジェクト」で作るか、既存のフォルダを追加してください。");
				return;
			}
			ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;
			if (!ImGui::BeginTable("projects", 5, flags)) return;
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthFixed, 200);
			ImGui::TableSetupColumn("エンジン", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("最後に開いた", ImGuiTableColumnFlags_WidthFixed, 140);
			ImGui::TableSetupColumn("場所", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80);
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < list.size(); i++)
			{
				auto& p = list[i];
				std::filesystem::path dir = u8path(p.path);
				bool exists = std::filesystem::exists(dir / "project.json");
				std::string engine = exists ? LauncherCore::projectEngine(dir) : "";
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetFrameHeight());
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				std::string name = u8str(dir.filename());
				if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_AllowDoubleClick)
					&& ImGui::IsMouseDoubleClicked(0) && exists)
					openProject(p.path, "");
				if (ImGui::BeginPopupContextItem("row")) { drawRowMenu(p.path, exists); ImGui::EndPopup(); }
				ImGui::TableNextColumn();
				if (!exists) ImGui::TextColored(COLOR_ERROR, "フォルダがありません");
				else if (engine.empty()) ImGui::TextColored(COLOR_WARN, "(engine 無し)");
				else if (!core.isInstalled(engine)) ImGui::TextColored(COLOR_WARN, "%s(未インストール)", engine.c_str());
				else ImGui::TextUnformatted(engine.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(p.lastOpened.empty() ? "-" : p.lastOpened.c_str());
				ImGui::TableNextColumn();
				ImGui::TextColored(COLOR_DIM, "%s", p.path.c_str());
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(!exists);
				if (ImGui::SmallButton("開く")) openProject(p.path, "");
				ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::SmallButton("…")) ImGui::OpenPopup("menu");
				if (ImGui::BeginPopup("menu")) { drawRowMenu(p.path, exists); ImGui::EndPopup(); }
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		// 右クリックと「…」で出す、行の操作
		void drawRowMenu(const std::string& path, bool exists)
		{
			std::filesystem::path dir = u8path(path);
			std::string error;
			ImGui::TextDisabled("%s", u8str(dir.filename()).c_str());
			ImGui::Separator();
			ImGui::BeginDisabled(!exists);
			if (ImGui::MenuItem("設定…"))
			{
				editProject = path;
				setName = u8str(dir.filename());
				setEngine = LauncherCore::projectEngine(dir);
				setScene = LauncherCore::projectValue(dir, "startScene");
				setPauseUI = LauncherCore::projectValue(dir, "pauseUI");
				setError.clear();
				settingsRequest = true;
			}
			if (ImGui::MenuItem("複製…"))
			{
				editProject = path;
				dupName = LauncherCore::duplicateName(dir);
				dupError.clear();
				duplicateRequest = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("エクスプローラーで開く") && !LauncherCore::revealInExplorer(dir, error)) setStatus(error, COLOR_ERROR);
			ImGui::BeginDisabled(vscode.empty());
			if (ImGui::MenuItem("VS Code で開く"))
			{
				if (LauncherCore::openInVSCode(dir, error)) setStatus("VS Code で開きました: " + path, COLOR_OK);
				else setStatus(error, COLOR_ERROR);
			}
			ImGui::EndDisabled();
			if (vscode.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("VS Code が見つかりません。\nコマンドで winget install Microsoft.VisualStudioCode を実行して、ランチャーを開き直してください");
			ImGui::EndDisabled();
			ImGui::Separator();
			if (ImGui::MenuItem("一覧から外す"))
			{
				if (core.removeProject(dir, error)) setStatus("一覧から外しました(フォルダは残っています): " + path, COLOR_OK);
				else setStatus(error, COLOR_ERROR);
			}
			ImGui::BeginDisabled(!exists);
			if (ImGui::MenuItem("ごみ箱へ移す…"))
			{
				editProject = path;
				trashError.clear();
				trashRequest = true;
			}
			ImGui::EndDisabled();
		}

		// 選択肢から選ぶ。noneLabel を渡すと「無し(空)」も選べる。今の値が選択肢に無ければそう出す
		static void comboOf(const char* id, std::string& value, const std::vector<std::string>& items, const char* noneLabel = nullptr)
		{
			bool known = value.empty() || std::find(items.begin(), items.end(), value) != items.end();
			std::string preview = value.empty() ? (noneLabel ? noneLabel : "") : value;
			if (!known) preview += "(見つかりません)";
			if (!ImGui::BeginCombo(id, preview.c_str())) return;
			if (noneLabel && ImGui::Selectable(noneLabel, value.empty())) value.clear();
			for (auto& item : items)
				if (ImGui::Selectable(item.c_str(), item == value)) value = item;
			ImGui::EndCombo();
		}

		void showError(const std::string& error)
		{
			if (error.empty()) return;
			ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ERROR);
			ImGui::TextWrapped("%s", error.c_str());
			ImGui::PopStyleColor();
		}

		void drawSettings()
		{
			if (settingsRequest)
			{
				ImGui::OpenPopup("設定");
				settingsRequest = false;
			}
			ImGui::SetNextWindowSize(ImVec2(640, 0));
			if (!ImGui::BeginPopupModal("設定", nullptr, ImGuiWindowFlags_NoResize)) return;
			std::filesystem::path dir = u8path(editProject);
			const float labelWidth = 150;
			auto label = [&](const char* text) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(text);
				ImGui::SameLine(labelWidth);
			};
			ImGui::TextColored(COLOR_DIM, "%s", editProject.c_str());
			ImGui::Spacing();

			label("名前(フォルダ名)");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##name", &setName);

			label("エンジン");
			ImGui::SetNextItemWidth(200);
			comboOf("##engine", setEngine, installed);

			label("startScene");
			ImGui::SetNextItemWidth(-1);
			comboOf("##scene", setScene, LauncherCore::projectScenes(dir));

			label("pauseUI");
			ImGui::SetNextItemWidth(-1);
			comboOf("##pause", setPauseUI, LauncherCore::projectUIs(dir), "(なし)");

			ImGui::Spacing();
			ImGui::TextDisabled("project.json のほかの項目はそのまま残します。直接直すなら「VS Code で開く」");
			bool validName = LauncherCore::validFolderName(setName);
			if (!validName) ImGui::TextColored(COLOR_ERROR, "フォルダ名に使えない名前です(\\ / : * ? \" < > | と、終わりの空白・点は使えません)");
			else if (setEngine == "0.1.0" && LauncherCore::hasNonAscii(u8str(dir.parent_path() / u8path(setName))))
				ImGui::TextColored(COLOR_WARN, "パスに日本語などが入っています。エディタ 0.1.0 はこのプロジェクトを開けません(英数字のパスにするか、0.1.1 以降を使ってください)");
			showError(setError);
			ImGui::Spacing();

			ImGui::BeginDisabled(!validName);
			if (ImGui::Button("保存"))
			{
				//project.json を先に直し、名前(フォルダ名)は最後に変える。名前を変えられなくても設定は残る
				std::string error;
				bool ok = true;
				const std::pair<const char*, const std::string*> fields[] = { { "engine", &setEngine }, { "startScene", &setScene }, { "pauseUI", &setPauseUI } };
				for (auto& [key, value] : fields)
				{
					if (ok && LauncherCore::projectValue(dir, key) != *value) ok = LauncherCore::setProjectValue(dir, key, *value, error);
				}
				std::filesystem::path renamed = dir;
				if (ok) ok = core.renameProject(dir, setName, error, &renamed);
				if (ok)
				{
					setStatus("保存しました: " + u8str(renamed), COLOR_OK);
					ImGui::CloseCurrentPopup();
				}
				else setError = error;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		void drawDuplicate()
		{
			if (duplicateRequest)
			{
				ImGui::OpenPopup("複製");
				duplicateRequest = false;
			}
			ImGui::SetNextWindowSize(ImVec2(640, 0));
			if (!ImGui::BeginPopupModal("複製", nullptr, ImGuiWindowFlags_NoResize)) return;
			std::filesystem::path dir = u8path(editProject);
			ImGui::TextColored(COLOR_DIM, "%s", editProject.c_str());
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("新しい名前");
			ImGui::SameLine(110);
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##dupname", &dupName);
			std::filesystem::path dest = dir.parent_path() / u8path(dupName);
			ImGui::TextColored(COLOR_DIM, "作る場所: %s", u8str(dest).c_str());
			ImGui::TextDisabled("build/(ゲームのコードのビルド結果)はコピーしません。開いた時に作り直します");
			if (LauncherCore::projectEngine(dir) == "0.1.0" && LauncherCore::hasNonAscii(u8str(dest)))
				ImGui::TextColored(COLOR_WARN, "パスに日本語などが入っています。エディタ 0.1.0 はこのプロジェクトを開けません");
			showError(dupError);
			ImGui::Spacing();
			ImGui::BeginDisabled(!LauncherCore::validFolderName(dupName));
			if (ImGui::Button("複製する"))
			{
				std::string error;
				if (core.duplicateProject(dir, dest, error))
				{
					setStatus("複製しました: " + u8str(dest), COLOR_OK);
					ImGui::CloseCurrentPopup();
				}
				else dupError = error;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		void drawTrash()
		{
			if (trashRequest)
			{
				ImGui::OpenPopup("ごみ箱へ移す");
				trashRequest = false;
			}
			ImGui::SetNextWindowSize(ImVec2(560, 0));
			if (!ImGui::BeginPopupModal("ごみ箱へ移す", nullptr, ImGuiWindowFlags_NoResize)) return;
			ImGui::TextWrapped("このプロジェクトのフォルダをごみ箱へ移し、一覧から外します。ごみ箱から元に戻せます。");
			ImGui::TextColored(COLOR_DIM, "%s", editProject.c_str());
			showError(trashError);
			ImGui::Spacing();
			if (ImGui::Button("ごみ箱へ移す"))
			{
				std::string error;
				if (core.trashProject(u8path(editProject), error))
				{
					setStatus("ごみ箱へ移しました: " + editProject, COLOR_OK);
					ImGui::CloseCurrentPopup();
				}
				else trashError = error;
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		void drawUninstall()
		{
			if (uninstallRequest)
			{
				ImGui::OpenPopup("バージョンを消す");
				uninstallRequest = false;
			}
			ImGui::SetNextWindowSize(ImVec2(600, 0));
			if (!ImGui::BeginPopupModal("バージョンを消す", nullptr, ImGuiWindowFlags_NoResize)) return;
			ImGui::TextWrapped("エンジン %s を消します(%s)。Releases か zip からいつでも入れ直せます。", uninstallVersion.c_str(), u8str(core.editorDir(uninstallVersion)).c_str());
			if (!uninstallUsers.empty())
			{
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARN);
				ImGui::TextWrapped("このバージョンを使っているプロジェクトがあります。開く時に、入れ直すか別のバージョンを選ぶことになります。");
				ImGui::PopStyleColor();
				for (auto& p : uninstallUsers)
				{
					ImGui::Bullet();
					ImGui::TextUnformatted(p.c_str());
				}
			}
			ImGui::Spacing();
			ImGui::BeginDisabled(busy());
			if (ImGui::Button("消す"))
			{
				std::string version = uninstallVersion;
				startJob([this, version](std::string& message, const LauncherCore::Progress& progress) {
					progress(version + " を消しています…");
					std::string error;
					if (!core.uninstall(version, error)) { message = error; return false; }
					message = version + " を消しました";
					return true;
				});
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		void openProject(const std::string& path, const std::string& engineOverride)
		{
			std::string error, missing;
			if (core.openProject(u8path(path), error, engineOverride, &missing))
			{
				std::string engine = engineOverride.empty() ? LauncherCore::projectEngine(u8path(path)) : engineOverride;
				setStatus("エディタ " + engine + " で開きました: " + path, COLOR_OK);
				return;
			}
			if (error.find("project.json がありません") != std::string::npos)
			{
				setStatus(error, COLOR_ERROR);
				return;
			}
			//入っていない・engine が無い時は、入れるか別のバージョンを選ぶ
			missingProject = path;
			missingVersion = missing;
			otherVersion = installed.empty() ? "" : installed.front();
			missingOpenRequest = true;
		}

		void drawMissingEngine()
		{
			if (missingOpenRequest)
			{
				ImGui::OpenPopup("エンジンが入っていません");
				missingOpenRequest = false;
			}
			ImGui::SetNextWindowSize(ImVec2(560, 0));
			if (!ImGui::BeginPopupModal("エンジンが入っていません", nullptr, ImGuiWindowFlags_NoResize)) return;
			ImGui::TextWrapped("%s", missingProject.c_str());
			if (missingVersion.empty()) ImGui::TextWrapped("project.json に engine(使うバージョン)がありません。開くバージョンを選んでください。");
			else ImGui::TextWrapped("このプロジェクトはエンジン %s 用ですが、%s は入っていません。", missingVersion.c_str(), missingVersion.c_str());
			ImGui::Spacing();

			if (!missingVersion.empty())
			{
				bool inReleases = std::any_of(releases.begin(), releases.end(), [&](auto& r) { return r.version == missingVersion; });
				ImGui::BeginDisabled(!gh.ok() || !inReleases || busy());
				if (ImGui::Button(("エンジン " + missingVersion + " を入れて開く").c_str()))
				{
					std::string version = missingVersion, path = missingProject;
					startJob([this, version](std::string& message, const LauncherCore::Progress& progress) {
						std::string error;
						if (!core.install(version, error, progress)) { message = error; return false; }
						message = version + " を入れました";
						return true;
					}, [this, path](bool ok) { if (ok) openProject(path, ""); });
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndDisabled();
				if (!gh.ok()) ImGui::TextColored(COLOR_WARN, "gh が使えないので入れられません(「バージョン」に理由)");
				else if (!inReleases && ghChecked) ImGui::TextColored(COLOR_WARN, "Releases に %s がありません", missingVersion.c_str());
				ImGui::Separator();
			}

			if (installed.empty()) ImGui::TextDisabled("入っているバージョンがありません");
			else
			{
				ImGui::SetNextItemWidth(160);
				if (ImGui::BeginCombo("##other", otherVersion.c_str()))
				{
					for (auto& v : installed)
						if (ImGui::Selectable(v.c_str(), v == otherVersion)) otherVersion = v;
					ImGui::EndCombo();
				}
				ImGui::SameLine();
				if (ImGui::Button("このバージョンで開く"))
				{
					ImGui::CloseCurrentPopup();
					openProject(missingProject, otherVersion);
				}
				ImGui::TextDisabled("project.json の engine は変えません(エディタは違うバージョンだと警告を出します)");
			}
			ImGui::Spacing();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		void beginNewProject()
		{
			newParent = core.projectsDir();
			newVersion = installed.empty() ? "" : installed.front();
			auto list = core.templates(newVersion);
			newTemplate = std::find(list.begin(), list.end(), "Demo") != list.end() ? "Demo" : (list.empty() ? "" : list.front());
			newError.clear();
			ImGui::OpenPopup("新しいプロジェクト"); //drawNewProject からだけ呼ぶ
		}

		void drawNewProject()
		{
			if (openNewOnStart && !installed.empty())
			{
				beginNewProject();
				openNewOnStart = false;
			}
			ImGui::SetNextWindowSize(ImVec2(640, 0));
			if (!ImGui::BeginPopupModal("新しいプロジェクト", nullptr, ImGuiWindowFlags_NoResize)) return;
			const float labelWidth = 110;
			auto label = [&](const char* text) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(text);
				ImGui::SameLine(labelWidth);
			};

			label("名前");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##name", &newName);

			label("置き場所");
			ImGui::SetNextItemWidth(-90);
			ImGui::InputText("##parent", &newParent);
			ImGui::SameLine();
			if (ImGui::Button("参照…"))
			{
				std::string dir = pickPath(true, newParent);
				if (!dir.empty()) newParent = dir;
			}

			label("エンジン");
			ImGui::SetNextItemWidth(200);
			if (ImGui::BeginCombo("##version", newVersion.c_str()))
			{
				for (auto& v : installed)
				{
					if (ImGui::Selectable(v.c_str(), v == newVersion))
					{
						newVersion = v;
						auto list = core.templates(v);
						if (std::find(list.begin(), list.end(), newTemplate) == list.end()) newTemplate = list.empty() ? "" : list.front();
					}
				}
				ImGui::EndCombo();
			}

			label("ひな形");
			ImGui::SetNextItemWidth(200);
			auto templates = core.templates(newVersion);
			if (ImGui::BeginCombo("##template", newTemplate.c_str()))
			{
				for (auto& t : templates)
					if (ImGui::Selectable(t.c_str(), t == newTemplate)) newTemplate = t;
				ImGui::EndCombo();
			}

			std::filesystem::path dest = u8path(newParent) / u8path(newName);
			ImGui::Spacing();
			ImGui::TextColored(COLOR_DIM, "作る場所: %s", u8str(dest).c_str());
			//エディタ 0.1.0 は起動オプションのパスを ANSI で読むので、日本語などの入ったパスのプロジェクトを開けない(0.1.1 で直した)
			std::string destText = u8str(dest);
			if (newVersion == "0.1.0" && std::any_of(destText.begin(), destText.end(), [](char c) { return static_cast<unsigned char>(c) >= 0x80; }))
				ImGui::TextColored(COLOR_WARN, "パスに日本語などが入っています。エディタ 0.1.0 はこのプロジェクトを開けません(英数字のパスにするか、0.1.1 以降を使ってください)");
			if (!newError.empty()) ImGui::TextColored(COLOR_ERROR, "%s", newError.c_str());
			ImGui::Spacing();

			bool valid = !newName.empty() && !newParent.empty() && !newVersion.empty() && !newTemplate.empty()
				&& newName.find_first_of("\\/:*?\"<>|") == std::string::npos;
			ImGui::BeginDisabled(!valid);
			bool create = ImGui::Button("作る");
			ImGui::SameLine();
			bool createOpen = ImGui::Button("作って開く");
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
			if (create || createOpen)
			{
				std::string error;
				if (core.createProject(dest, newVersion, newTemplate, error))
				{
					setStatus("作りました: " + u8str(dest), COLOR_OK);
					ImGui::CloseCurrentPopup();
					if (createOpen) openProject(u8str(dest), "");
				}
				else newError = error;
			}
			ImGui::EndPopup();
		}

		// ---------------------------------------------------- バージョン
		void drawVersions()
		{
			ImGui::SeparatorText("入れたバージョン");
			if (installed.empty()) ImGui::TextDisabled("まだありません。下の Releases か zip から入れてください。");
			for (auto& v : installed)
			{
				ImGui::PushID(v.c_str());
				ImGui::AlignTextToFramePadding();
				ImGui::Bullet();
				ImGui::TextUnformatted(v.c_str());
				ImGui::SameLine(160);
				ImGui::BeginDisabled(busy());
				if (ImGui::SmallButton("消す…"))
				{
					uninstallVersion = v;
					uninstallUsers = core.projectsUsing(v);
					uninstallRequest = true;
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::TextColored(COLOR_DIM, "%s", u8str(core.editorDir(v)).c_str());
				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::SeparatorText("入れる");
			ImGui::BeginDisabled(busy());
			if (ImGui::Button("zip から入れる…"))
			{
				std::string zip = pickPath(false, "");
				if (!zip.empty())
				{
					startJob([this, zip](std::string& message, const LauncherCore::Progress& progress) {
						std::string version, error;
						if (!core.installZip(u8path(zip), version, error, progress)) { message = error; return false; }
						message = version + " を入れました";
						return true;
					});
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("再確認")) checkGhAsync();
			ImGui::EndDisabled();
			ImGui::Spacing();

			if (!ghChecked)
			{
				ImGui::TextDisabled("gh と Releases を確かめています…");
				return;
			}
			if (!gh.ok())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARN);
				ImGui::TextWrapped("%s", gh.message.c_str());
				ImGui::PopStyleColor();
				return;
			}
			if (!releasesError.empty())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ERROR);
				ImGui::TextWrapped("%s", releasesError.c_str());
				ImGui::PopStyleColor();
				return;
			}
			ImGui::TextColored(COLOR_DIM, "Releases(%s)", LauncherCore::RELEASE_REPO);
			if (!ImGui::BeginTable("releases", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) return;
			ImGui::TableSetupColumn("バージョン", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("公開日", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			for (auto& r : releases)
			{
				ImGui::PushID(r.version.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(r.version.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(r.published.c_str());
				ImGui::TableNextColumn();
				if (core.isInstalled(r.version)) ImGui::TextColored(COLOR_OK, "入れ済み");
				else
				{
					ImGui::BeginDisabled(busy());
					if (ImGui::SmallButton("入れる"))
					{
						std::string version = r.version;
						startJob([this, version](std::string& message, const LauncherCore::Progress& progress) {
							std::string error;
							if (!core.install(version, error, progress)) { message = error; return false; }
							message = version + " を入れました";
							return true;
						});
					}
					ImGui::EndDisabled();
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		void drawStatus()
		{
			ImGui::Separator();
			if (busy())
			{
				std::lock_guard lock(jobMutex);
				const char* spin = "|/-\\";
				ImGui::Text("%c %s", spin[static_cast<int>(ImGui::GetTime() * 8) % 4], jobProgress.c_str());
			}
			else if (!statusText.empty()) ImGui::TextColored(statusColor, "%s", statusText.c_str());
			else ImGui::TextColored(COLOR_DIM, "置き場所: %s", u8str(core.home()).c_str());
		}
	};
}

int runGui(LauncherCore& core, const LauncherOptions& options)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!glfwInit())
	{
		std::cerr << "glfwInit に失敗しました" << std::endl;
		return 1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1100, 680, "G.O.M.I Launcher " LAUNCHER_VERSION, nullptr, nullptr);
	if (!window)
	{
		std::cerr << "窓を作れませんでした" << std::endl;
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr; //窓は1枚だけなので並びは覚えない
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	const char* fonts[] = { "C:/Windows/Fonts/meiryo.ttc", "C:/Windows/Fonts/YuGothM.ttc", "C:/Windows/Fonts/msgothic.ttc" };
	for (auto f : fonts)
	{
		if (std::filesystem::exists(f) && io.Fonts->AddFontFromFileTTF(f, 17.0f)) break;
	}
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 3;
	ImGui::GetStyle().FramePadding = ImVec2(8, 5);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	int result = 0;
	{
		LauncherApp app(core, options);
		int frame = 0;
		while (!glfwWindowShouldClose(window))
		{
			//台本(--frames)の時は待たずに回す。普段は入力か 0.1 秒ごとに描き直す(進み具合の表示のため)
			if (options.frames > 0) glfwPollEvents();
			else glfwWaitEventsTimeout(0.1);
			//台本の時は、裏の仕事(gh を見る・入れる)の間はコマを数えない(撮る画面を毎回同じにする)
			bool counting = !(options.frames > 0 && app.busy());
			if (counting) frame++;
			else std::this_thread::sleep_for(std::chrono::milliseconds(10));
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			if (counting)
			{
				//エディタと同じく、1つ前のコマでカーソルを置き、押して、次のコマで離す
				for (auto& c : options.clicks)
				{
					if (frame >= c.frame - 1 && frame <= c.frame + 1) io.AddMousePosEvent(c.x, c.y);
					if (frame == c.frame) io.AddMouseButtonEvent(0, true);
					if (frame == c.frame + 1) io.AddMouseButtonEvent(0, false);
				}
			}
			ImGui::NewFrame();
			app.draw();
			ImGui::Render();
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			glViewport(0, 0, w, h);
			glClearColor(0.1f, 0.1f, 0.12f, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			for (auto& [at, path] : options.screenshots)
			{
				if (counting && at == frame && !saveScreenshot(window, path)) std::cerr << "screenshot failed: " << path << std::endl;
			}
			glfwSwapBuffers(window);
			if (options.frames > 0 && frame >= options.frames) break;
		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	CoUninitialize();
	return result;
}
