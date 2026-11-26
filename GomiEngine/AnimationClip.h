#pragma once
#include "AssetDatabase.h"

#include <array>
#include <vector>

/// <summary>
/// クリップの1コマ。image の uv の範囲を描く(連番は画像全体、シートは1マス分)
/// </summary>
struct AnimationFrame
{
	AssetID image;
	std::array<float, 4> uv = { 0, 0, 1, 1 };
};

/// <summary>
/// コマ送りの素材(assets/animations/*.json)。
///   シート: { "sheet": "images/a.png", "columns": 4, "rows": 2, "count": 8, "fps": 12, "loop": true }
///   連番  : { "frames": ["images/a_00.png", ...], "fps": 12, "loop": false }
/// どちらも任意で "durations": [1, 1, 2, ...](コマごとの長さ。fps の1コマ単位。省略で全部 1)。fps は省略で 12、loop は省略で true
/// </summary>
struct AnimationClip
{
	std::vector<AnimationFrame> frames;
	std::vector<float> durations; //frames と同じ数。全部 0 より大きい
	float fps = 12;
	bool loop = true;

	/// <summary>全部のコマの長さ(秒)</summary>
	float duration() const;
	/// <summary>time 秒の時のコマの番号。ループしなければ最後のコマで止まる</summary>
	int frameAt(float time) const;
	/// <summary>ループしないクリップが最後まで流れ終わったか(ループするものは常に false)</summary>
	bool finishedAt(float time) const;

	/// <summary>
	/// JSON から作る。知らないキー・足りない項目・存在しない画像・連番の大きさ違いは、source 入りの例外
	/// </summary>
	static AnimationClip parse(const Json& json, const std::string& source);
	/// <summary>
	/// assets の ID から読む(読んだものは覚える)。読めなければ警告を1回出して nullptr
	/// </summary>
	static const AnimationClip* load(const AssetID& path);
	/// <summary>覚えているクリップを捨てる(プロジェクトを開き直す時)</summary>
	static void clearCache();
};
