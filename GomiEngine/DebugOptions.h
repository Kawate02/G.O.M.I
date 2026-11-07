#pragma once
#include <optional>
#include <string>
#include <vector>
#include "InputState.h"

/// <summary>
/// 動作確認用の起動オプション。人が触らなくても画面遷移や操作を再現してスクリーンショットを残せる
///   --world <名前>                 最初に開く World(省略時は最初に登録したもの)
///   --frames <n>                   n フレーム目で終了
///   --fixed-dt <fps>               deltaTime を 1/fps 秒に固定する(実行のたびに同じ画面になる)
///   --screenshot <frame>:<path>    そのフレームの画面を png で保存(複数可)
///   --key <frame>[-<end>]:<キー名>   そのフレーム(から end まで)キーを押したことにする(InputKey の名前。KEY_ は省略可)
///   --click <frame>:<x>,<y>        その位置で左クリックしたことにする
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
	std::vector<Screenshot> screenshots;
	void pressAt(InputState& state, InputKey key, int frame, int start, int end) const;
public:
	std::string startWorld;
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
	inline bool shouldQuit(int frame) const { return maxFrames > 0 && frame >= maxFrames; }
};
