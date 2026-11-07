#pragma once
#include <string>
#include <cstddef>
#include "TextAnchor.h"

struct ImageResource
{
	int handle = -1;
	int width = 0;
	int height = 0;
};

/// <summary>
/// 描画システムの抽象レイヤー
/// </summary>
class IDrawer
{
public:
    virtual void init() = 0;
	virtual void clearScreen() = 0;
	/// <summary>
	/// 画像ファイル(png等)のバイト列からテクスチャを作る。失敗したら handle = -1
	/// </summary>
	virtual ImageResource createTexture(const unsigned char* fileData, size_t size) = 0;
	/// <summary>
	/// (x, y) を左上に w x h の大きさで描く。angle(ラジアン、時計回り)は中心で回す
	/// </summary>
    virtual void drawImage(int handle, float x, float y, float w, float h, float angle) = 0;
    virtual void drawText(const std::string& text, int fontSize, Anchor anchor, float x, float y) = 0;
	/// <summary>
	/// 今描いた画面を画像ファイルに保存する(present の前に呼ぶ)。未対応なら false
	/// </summary>
	virtual bool captureScreen(const std::string& path) { return false; }
    virtual void present() = 0;
    virtual ~IDrawer() = default;
};
