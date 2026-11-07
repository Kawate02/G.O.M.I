#pragma once
#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "TextAnchor.h"

struct ImageResource
{
	int handle = -1;
	int width = 0;
	int height = 0;
};

/// <summary>
/// メッシュの頂点(1頂点 = 位置 xyz・法線 xyz・UV の8要素)と三角形の頂点番号
/// </summary>
struct MeshData
{
	static constexpr int STRIDE = 8;
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
};

/// <summary>
/// 3D を描く時の共通の設定
/// </summary>
struct Scene3DParams
{
	glm::mat4 view{ 1.0f };
	float fov = 60;       //縦の視野角(度)
	float nearClip = 0.1f;
	float farClip = 1000;
	glm::vec3 lightDirection{ 0, -1, 0 }; //光が進む向き
	glm::vec3 lightColor{ 1, 1, 1 };
	float ambient = 0.25f;
};

/// <summary>
/// 描画システムの抽象レイヤー
/// </summary>
class IDrawer
{
public:
    virtual void init() = 0;
	/// <summary>
	/// 画面を色で塗りつぶし、奥行きもリセットする
	/// </summary>
	virtual void clearScreen(float r, float g, float b) = 0;
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
	/// メッシュを GPU に載せてハンドルを返す。失敗したら -1
	/// </summary>
	virtual int createMesh(const MeshData& mesh) = 0;
	/// <summary>
	/// ここから endScene3D までの drawMesh を、奥行き付き・光の当たった 3D として描く
	/// </summary>
	virtual void beginScene3D(const Scene3DParams& params) = 0;
	/// <summary>
	/// texture が -1 なら色だけで塗る
	/// </summary>
	virtual void drawMesh(int mesh, int texture, const glm::mat4& model, const glm::vec3& color) = 0;
	virtual void endScene3D() = 0;

	/// <summary>
	/// 画面の外の描き先(w x h)を作る。エディタでゲームの画面をパネルに出す時に使う。未対応・失敗なら -1
	/// </summary>
	virtual int createRenderTarget(int w, int h) { return -1; }
	/// <summary>
	/// 以降の描画先を切り替える。-1 でウィンドウに戻す。2D の座標と 3D の縦横比は描き先の大きさになる
	/// </summary>
	virtual void setRenderTarget(int target) {}
	/// <summary>
	/// 描き先のテクスチャの番号(OpenGL ならテクスチャ名)。ImGui::Image などに渡す。上下は逆さま
	/// </summary>
	virtual uintptr_t renderTargetTexture(int target) { return 0; }

	/// <summary>
	/// 今描いた画面を画像ファイルに保存する(present の前に呼ぶ)。描き先を切り替えていればその中身。未対応なら false
	/// </summary>
	virtual bool captureScreen(const std::string& path) { return false; }
    virtual void present() = 0;
    virtual ~IDrawer() = default;
};
