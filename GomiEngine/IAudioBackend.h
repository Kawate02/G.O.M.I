#pragma once
#include <string>
#include <cstddef>

/// <summary>
/// 音声の抽象レイヤー。volume は 0〜255(DxLib時代と同じ単位)
/// </summary>
class IAudioBackend
{
public:
	virtual void init() = 0;
	/// <summary>
	/// 音声ファイル(wav等)のバイト列を登録して音のハンドルを返す。data は呼び出し側が生かし続ける。失敗したら -1
	/// </summary>
	virtual int load(const std::string& name, const void* data, size_t size) = 0;
	/// <summary>
	/// 音を1回鳴らし、再生中の音(ボイス)のIDを返す。失敗したら -1
	/// </summary>
	virtual int play(int sound, float volume) = 0;
	virtual bool isPlaying(int voice) = 0;
	virtual void stop(int voice) = 0;
	/// <summary>
	/// 鳴り終わったボイスを片付ける。毎フレーム呼ぶ
	/// </summary>
	virtual void update() = 0;
	virtual ~IAudioBackend() = default;
};
