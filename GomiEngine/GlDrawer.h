#pragma once
#include "IDrawer.h"
#include <memory>
#include <unordered_map>
#include <vector>

class GlfwWindow;
struct FontAtlas;

/// <summary>
/// OpenGL 4.5 による2D描画。スプライトは1枚ずつ四角形として描く
/// </summary>
class GlDrawer : public IDrawer
{
private:
	GlfwWindow& window;
	unsigned int program = 0;
	unsigned int vao = 0;
	unsigned int vbo = 0;
	int locRect = -1;
	int locAngle = -1;
	int locScreen = -1;
	int locUV = -1;
	int locColor = -1;
	int locMode = -1;

	std::vector<unsigned int> textures; // ハンドル = この配列の添字
	std::vector<unsigned char> fontData;
	int fontOffset = 0;
	bool fontTried = false;
	std::unordered_map<int, std::unique_ptr<FontAtlas>> fonts;

	FontAtlas* getFont(int size);
	void drawQuad(unsigned int texture, float x, float y, float w, float h, float angle, const float uv[4], int mode);
public:
	explicit GlDrawer(GlfwWindow& window);
	~GlDrawer() override;
	void init() override;
	void clearScreen() override;
	ImageResource createTexture(const unsigned char* fileData, size_t size) override;
	void drawImage(int handle, float x, float y, float w, float h, float angle) override;
	void drawText(const std::string& text, int fontSize, Anchor anchor, float x, float y) override;
	bool captureScreen(const std::string& path) override;
	void present() override;
};
