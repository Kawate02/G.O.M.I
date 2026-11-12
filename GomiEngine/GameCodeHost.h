#pragma once
#include "GameModule.h"

#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/// <summary>
/// エディタ用: プロジェクトのゲームのコード(game/)の保存を見張って GomiGame.dll を裏でビルドし、
/// 使用中で上書きできないよう連番のコピーにして読む。差し替え自体(World の作り直し)は SceneEditor::swapCode
/// </summary>
class GameCodeHost
{
public:
	struct Config
	{
		std::filesystem::path projectDir; //プロジェクトのフォルダ(cmake に GOMI_PROJECT で渡す)
		std::filesystem::path sourceDir;  //<プロジェクト>/game
		std::filesystem::path sdkDir;     //エンジンの sdk(sdk/game/CMakeLists.txt を cmake -S に渡す)
		std::filesystem::path cmake;      //cmake.exe。空なら findCMake で探す
		std::filesystem::path buildDir;   //<プロジェクト>/build(cmake -B / --build に渡す)
		std::string buildConfig;          //Debug など(エンジンと同じ構成)
		std::string target = "GomiGame";
		std::filesystem::path builtDll;   //ビルドで出来る DLL(<build>/bin/GomiGame.dll)
		std::filesystem::path liveDir;    //読むためのコピーの置き場

		/// <summary>プロジェクトのフォルダとエンジンの sdk から、上の場所を全部決める</summary>
		static Config forProject(const std::filesystem::path& projectDir, const std::filesystem::path& sdkDir, const std::string& buildConfig);
	};
	struct BuildResult
	{
		bool ok = false;
		std::string output;              //ビルドの出力の全文
		std::vector<std::string> errors; //エラーの行(重複を除く)
		double seconds = 0;
	};
private:
	Config config;
	GameModule gameModule;
	std::filesystem::path previousCopy; //戻す用(今読んでいるものの1つ前)
	int copyNumber = 0;
	std::filesystem::file_time_type loadedStamp{}; //読んだ時のビルド済み DLL の更新時刻

	std::map<std::filesystem::path, std::filesystem::file_time_type> stamps;
	double lastPoll = -100;
	bool changed = false;     //前回見た時から変わった(落ち着いたらビルドする)
	bool buildAgain = false;  //ビルド中に変わった

	std::thread worker;
	std::atomic<bool> building{ false };
	std::mutex resultMutex;
	std::optional<BuildResult> result;

	std::map<std::filesystem::path, std::filesystem::file_time_type> scan() const;
	bool copyAndLoad(std::string& error);
public:
	explicit GameCodeHost(Config config);
	~GameCodeHost();
	GameCodeHost(const GameCodeHost&) = delete;
	GameCodeHost& operator=(const GameCodeHost&) = delete;

	/// <summary>ゲームのコードのフォルダがある(無ければ何もしない)</summary>
	bool enabled() const;
	/// <summary>起動時: ビルド済みの DLL があれば読む(まだ World が無い時に呼ぶ)</summary>
	bool loadBuilt(std::string& error);
	/// <summary>game/ にソースがあるのに、まだビルドした DLL が無い(作ったばかりのプロジェクトなど)</summary>
	bool needsFirstBuild() const;
	/// <summary>ビルド済みの DLL が、今読んでいるものと違う(ビルドで作り直された)</summary>
	bool builtChanged() const;
	/// <summary>毎フレーム呼ぶ。0.5 秒ごとに保存を見て、落ち着いたらビルドを始める</summary>
	void poll(double now);
	void startBuild();
	inline bool isBuilding() const { return building; }
	/// <summary>終わったビルドの結果(1回だけ返す)</summary>
	std::optional<BuildResult> takeResult();

	/// <summary>今の DLL を外してビルドしたものを読む(SceneEditor::swapCode の swap に渡す)</summary>
	bool swapToBuilt(std::string& error);
	/// <summary>前に読んでいたものに戻す(SceneEditor::swapCode の revert に渡す)</summary>
	void revert();
	inline const GameModule& module() const { return gameModule; }

	enum class TemplateKind { Component, System };
	/// <summary>
	/// game/ にひな形を作る。Component は X.h(型)と X.cpp(登録)、System は X.cpp。
	/// name は "Hp" のような名前(後ろの Component / System は付けても付けなくてもよい)。作ったファイルを返す(最初が開く物)
	/// </summary>
	std::vector<std::filesystem::path> createTemplate(TemplateKind kind, const std::string& name, std::string& error) const;
	/// <summary>VS Code でフォルダを開き、file があればそのファイルも開く。VS Code が無ければ既定のアプリで開く</summary>
	static bool openInCodeEditor(const std::filesystem::path& folder, const std::filesystem::path& file, std::string& error);
	inline const Config& settings() const { return config; }

	/// <summary>
	/// cmake.exe を探す。PATH > Visual Studio に付いてくるもの(vswhere で場所を引く)。無ければ空
	/// </summary>
	static std::filesystem::path findCMake();
};
