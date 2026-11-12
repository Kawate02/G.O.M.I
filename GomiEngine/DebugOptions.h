#pragma once
#include <optional>
#include <string>
#include <vector>
#include "InputState.h"

/// <summary>
/// 動作確認用の起動オプション。人が触らなくても画面遷移や操作を再現してスクリーンショットを残せる
///   --project <フォルダ>           開くプロジェクト(project.json のあるフォルダ。省略時は exe の隣、無ければ見本のプロジェクト)
///   --world <名前>                 最初に開く World(省略時は最初に登録したもの)
///   --frames <n>                   n フレーム目で終了
///   --fixed-dt <fps>               deltaTime を 1/fps 秒に固定する(実行のたびに同じ画面になる)
///   --screenshot <frame>:<path>    そのフレームの画面を png で保存(複数可)
///   --save-scene <frame>:<path>    そのフレームの更新前に World をシーンの JSON で保存(--world <path> で読める)
///   --key <frame>[-<end>]:<キー名>   そのフレーム(から end まで)キーを押したことにする(InputKey の名前。KEY_ は省略可)
///   --click <frame>:<x>,<y>        その位置で左クリックしたことにする
///   --drag <start>-<end>:<x1>,<y1>,<x2>,<y2>  左ボタンを押したまま動かす(エディタ用)
///   --type <frame>:<文字>          文字を入力したことにする(エディタの入力欄用)。{ENTER} {SELECTALL} や {CTRL+SHIFT+P} {DOWN} {ESC} のようにキーも押せる
///   --command <frame>:<ID>[=<引数>]  エディタのコマンドを実行する(例: 30:scene.open=Field、30:entity.select=player)
///   --version                      エンジンのバージョン(例: 0.1.0)を出して終わる
///   --list-commands                エディタのコマンドの一覧(ID・ショートカット・名前・引数)を出して終わる
///   --select <番号>                エディタで最初に選んでおく Entity の並び順の番号
///   --record <path>                操作を記録し、終わる時に記録の JSON を書く(Δt は 1/60 に固定。--fixed-dt があればそれ)
///   --replay <path>                記録を流し、確認点で画面と状態を基準(記録と同じ名前のフォルダ)と比べる。違えば終了コード 1
///                                  シーン・Δt・乱数の種・終わるフレームは記録のものを使う
///   --replay-update                --replay で比べずに、基準を今回の画面と状態で書き直す
///   --replay-out <フォルダ>        比べた結果(今回の画面・状態・差分の画像)の置き場。省略時は一時フォルダの gomi-replay/<記録名>
///   --no-wait                      フレームの間を待たずに流す(テストを速く回す)
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
	std::vector<Screenshot> commands;
	void pressAt(InputState& state, InputKey key, int frame, int start, int end) const;
public:
	std::string startWorld;
	std::string projectDir;
	int select = -1;
	int maxFrames = 0;
	float fixedDeltaTime = 0.0f; //0 なら実時間
	std::string recordPath;
	std::string replayPath;
	std::string replayOut;
	bool replayUpdate = false;
	bool noWait = false;
	bool listCommands = false;
	bool printVersion = false;

	/// <summary>
	/// 解釈できない引数があれば説明を返す
	/// </summary>
	std::optional<std::string> parse(int argc, char** argv);
	/// <summary>
	/// UTF-8 の引数の並び(先頭は実行ファイル)から読む
	/// </summary>
	std::optional<std::string> parse(const std::vector<std::string>& argv);
	/// <summary>
	/// 実際の入力の上に、このフレームで押したことにするキー・クリックを重ねる
	/// </summary>
	void apply(int frame, InputState& state) const;
	std::vector<std::string> screenshotsAt(int frame) const;
	std::vector<std::string> sceneSavesAt(int frame) const;
	std::vector<std::string> typedAt(int frame) const;
	/// <summary>
	/// このフレームに実行するエディタのコマンド("ID" か "ID=引数")
	/// </summary>
	std::vector<std::string> commandsAt(int frame) const;
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
