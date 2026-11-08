#pragma once
#include "InputState.h"
#include "JsonConvert.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

/// <summary>
/// 操作の記録(テキストの JSON)。始めるシーン・Δt・乱数の種と、フレームごとの入力の変化を持つ。
/// 同じ記録を流せば毎回同じ画面・同じ状態になるので、そのまま再現テストになる
/// </summary>
class Replay
{
public:
	/// <summary>
	/// そのフレームで変わった入力だけを持つ
	/// </summary>
	struct Event
	{
		int frame = 0;
		std::vector<InputKey> press;
		std::vector<InputKey> release;
		std::optional<Vec2> mouse;
		int wheel = 0;
	};
	std::string scene; //始めるシーン(空なら project.json の startScene)
	int fps = 60; //Δt = 1/fps に固定して流す
	uint32_t seed = 1; //Random の種
	int length = 0; //このフレームまで流して終わる
	std::vector<int> checkpoints; //画面と状態を基準と比べるフレーム
	std::vector<Event> events;

	inline float deltaTime() const { return 1.0f / fps; }
	inline bool isCheckpoint(int frame) const { return std::find(checkpoints.begin(), checkpoints.end(), frame) != checkpoints.end(); }

	Json toJson() const;
	/// <summary>
	/// 知らない項目・キー名があれば例外
	/// </summary>
	static Replay fromJson(const Json& j);
	static Replay load(const std::filesystem::path& path);
	void save(const std::filesystem::path& path) const;
	/// <summary>
	/// 基準(画面と状態)を置くフォルダ。記録の json と同じ名前のフォルダ
	/// </summary>
	static std::filesystem::path expectedDir(const std::filesystem::path& replayPath);
};

/// <summary>
/// 毎フレームの入力を受け取り、変わった所だけを記録する
/// </summary>
class ReplayRecorder
{
private:
	Replay replay;
	InputState last;
	bool first = true;
public:
	ReplayRecorder(const std::string& scene, int fps, uint32_t seed);
	void record(int frame, const InputState& state);
	/// <summary>
	/// 終わりのフレームを入れ、checkpointEvery フレームごとと最後を確認点にして返す
	/// </summary>
	Replay finish(int length, int checkpointEvery = 120) const;
	inline int eventCount() const { return static_cast<int>(replay.events.size()); }
};

/// <summary>
/// 記録から毎フレームの入力を作り直す。1フレーム目から順に呼ぶ
/// </summary>
class ReplayPlayer
{
private:
	Replay replay;
	size_t next = 0;
	InputState state;
	bool started = false;
public:
	explicit ReplayPlayer(Replay r) : replay(std::move(r)) {}
	/// <summary>
	/// out をこのフレームの記録の入力で丸ごと置き換える(実際のキー・マウスは使わない)
	/// </summary>
	void apply(int frame, InputState& out);
	inline const Replay& getReplay() const { return replay; }
};

/// <summary>
/// 確認点1つの比べた結果
/// </summary>
struct ReplayCheckResult
{
	int frame = 0;
	bool stateMissing = false; //基準の状態が無い
	std::vector<std::string> stateDiffs; //違う項目(多い時は途中まで)
	bool imageMissing = false; //基準の画面が無い・読めない
	bool sizeMismatch = false;
	long long pixelDiffs = 0; //違う画素の数
	std::string actualImage;
	std::string diffImage; //違う画素を赤くした画像(違いがある時だけ)
	inline bool stateOk() const { return !stateMissing && stateDiffs.empty(); }
	inline bool imageOk() const { return !imageMissing && !sizeMismatch && pixelDiffs == 0; }
	inline bool ok() const { return stateOk() && imageOk(); }
};

/// <summary>
/// 確認点で画面と状態を基準と比べる。Update では比べずに基準を書き直す
/// </summary>
class ReplayCheck
{
public:
	enum class Mode
	{
		Check,
		Update,
	};
private:
	std::filesystem::path expected;
	std::filesystem::path out;
	Mode mode;
	std::vector<ReplayCheckResult> results;
public:
	ReplayCheck(std::filesystem::path expectedDir, std::filesystem::path outDir, Mode mode);
	/// <summary>
	/// state は比べる状態の JSON、capture は画面を path に png で保存する関数
	/// </summary>
	const ReplayCheckResult& checkpoint(int frame, const Json& state, const std::function<bool(const std::string& path)>& capture);
	/// <summary>
	/// 確認点が全部済んでいて、全部一致したら true
	/// </summary>
	bool passed(const Replay& replay) const;
	/// <summary>
	/// 結果を人が読める行にする
	/// </summary>
	std::vector<std::string> report(const Replay& replay, const std::string& name) const;
	inline const std::vector<ReplayCheckResult>& getResults() const { return results; }
	inline Mode getMode() const { return mode; }
	inline const std::filesystem::path& getOutDir() const { return out; }

	/// <summary>
	/// 違う所を「パス: 基準 → 今回」の形で limit 件まで返す
	/// </summary>
	static std::vector<std::string> jsonDiff(const Json& expected, const Json& actual, size_t limit = 20);
	/// <summary>
	/// 2枚の png の違う画素を数える。diffPath が空でなく違いがあれば差分画像を書く。読めなければ -1、大きさが違えば -2
	/// </summary>
	static long long compareImages(const std::string& expectedPath, const std::string& actualPath, const std::string& diffPath);
	static std::string fileStem(int frame);
};
