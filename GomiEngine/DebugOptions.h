#pragma once
#include <optional>
#include <string>
#include <vector>
#include "InputState.h"

/// <summary>
/// 動作確認用の起動オプション。人が触らなくても画面遷移や操作を再現してスクリーンショットを残せる
///   --assets <フォルダ>            assets フォルダ(省略時は exe の隣、無ければソースの assets)
///   --world <名前>                 最初に開く World(省略時は最初に登録したもの)
///   --frames <n>                   n フレーム目で終了
///   --fixed-dt <fps>               deltaTime を 1/fps 秒に固定する(実行のたびに同じ画面になる)
///   --screenshot <frame>:<path>    そのフレームの画面を png で保存(複数可)
///   --save-scene <frame>:<path>    そのフレームの更新前に World をシーンの JSON で保存(--world <path> で読める)
///   --key <frame>[-<end>]:<キー名>   そのフレーム(から end まで)キーを押したことにする(InputKey の名前。KEY_ は省略可)
///   --click <frame>:<x>,<y>        その位置で左クリックしたことにする
///   --drag <start>-<end>:<x1>,<y1>,<x2>,<y2>  左ボタンを押したまま動かす(エディタ用)
///   --type <frame>:<文字>          文字を入力したことにする(エディタの入力欄用)
///   --select <番号>                エディタで最初に選んでおく Entity の並び順の番号
/// </summary>
class DebugOptions
{
private:
	struct KeyEvent
	{
		int start;
		int end;
		InputKey key;
	};
	struct ClickEvent
	{
		int frame;
		Vec2 pos;
	};
	struct Screenshot
	{
		int frame;
		std::string path;
	};
	std::vector<KeyEvent> keys;
	std::vector<ClickEvent> clicks;
	struct DragEvent
	{
		int start;
		int end;
		Vec2 from;
		Vec2 to;
	};
	std::vector<DragEvent> drags;
	std::vector<Screenshot> screenshots;
	std::vector<Screenshot> sceneSaves;
	std::vector<Screenshot> typed;
	void pressAt(InputState& state, InputKey key, int frame, int start, int end) const;
public:
	std::string startWorld;
	std::string assetDir;
	int select = -1;
	int maxFrames = 0;
	float fixedDeltaTime = 0.0f; //0 なら実時間

	/// <summary>
	/// 解釈できない引数があれば説明を返す
	/// </summary>
	std::optional<std::string> parse(int argc, char** argv);
	/// <summary>
	/// 実際の入力の上に、このフレームで押したことにするキー・クリックを重ねる
	/// </summary>
	void apply(int frame, InputState& state) const;
	std::vector<std::string> screenshotsAt(int frame) const;
	std::vector<std::string> sceneSavesAt(int frame) const;
	std::vector<std::string> typedAt(int frame) const;
	/// <summary>
	/// このフレームのクリック(位置を合わせるフレーム・押すフレーム・離すフレーム)。エディタが ImGui に渡す
	/// </summary>
	struct ClickState
	{
		bool move = false;
		bool press = false;
		bool release = false;
		Vec2 pos;
	};
	ClickState clickAt(int frame) const;
	inline bool shouldQuit(int frame) const { return maxFrames > 0 && frame >= maxFrames; }
};
