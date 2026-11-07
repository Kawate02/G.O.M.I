#include "GlDrawer.h"
#include "GlfwWindow.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_truetype.h>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

struct FontAtlas
{
	unsigned int texture = 0;
	int dim = 0;
	float ascent = 0;
	stbtt_bakedchar chars[96]; // ' '(32) 〜 '~'(126)
};

namespace
{
	const char* vertexSrc = R"(#version 450 core
layout(location = 0) in vec2 aPos;
uniform vec4 uRect;   // x, y, w, h (左上基準・ピクセル)
uniform float uAngle; // 中心まわりの回転(ラジアン、y下向きなので時計回り)
uniform vec2 uScreen; // 論理画面サイズ
uniform vec4 uUV;     // u0, v0, u1, v1
out vec2 vUV;
void main()
{
	vec2 local = (aPos - 0.5) * uRect.zw;
	float c = cos(uAngle);
	float s = sin(uAngle);
	vec2 p = vec2(local.x * c - local.y * s, local.x * s + local.y * c) + uRect.xy + uRect.zw * 0.5;
	gl_Position = vec4(p.x / uScreen.x * 2.0 - 1.0, 1.0 - p.y / uScreen.y * 2.0, 0.0, 1.0);
	vUV = mix(uUV.xy, uUV.zw, aPos);
}
)";

	const char* fragmentSrc = R"(#version 450 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform vec4 uColor;
uniform int uMode; // 0: 画像, 1: 文字(赤チャンネルを不透明度に使う)
void main()
{
	vec4 t = texture(uTex, vUV);
	FragColor = uMode == 1 ? vec4(uColor.rgb, uColor.a * t.r) : t * uColor;
}
)";

	const char* vertex3DSrc = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat3 uNormalMat;
out vec3 vNormal;
out vec2 vUV;
void main()
{
	gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
	vNormal = uNormalMat * aNormal;
	vUV = aUV;
}
)";

	// 拡散光(ランバート)+ 環境光
	const char* fragment3DSrc = R"(#version 450 core
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform vec3 uColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
void main()
{
	vec4 base = texture(uTex, vUV) * vec4(uColor, 1.0);
	if (base.a < 0.5) discard; //抜き(透明部分)は描かない。半透明はまだ扱わない
	vec3 n = normalize(vNormal);
	float diffuse = max(dot(n, -normalize(uLightDir)), 0.0);
	FragColor = vec4(base.rgb * (uAmbient + diffuse * uLightColor), base.a);
}
)";

	unsigned int link(unsigned int vs, unsigned int fs)
	{
		unsigned int p = glCreateProgram();
		glAttachShader(p, vs);
		glAttachShader(p, fs);
		glLinkProgram(p);
		glDeleteShader(vs);
		glDeleteShader(fs);
		int ok = 0;
		glGetProgramiv(p, GL_LINK_STATUS, &ok);
		if (!ok) throw std::runtime_error("shader link failed");
		return p;
	}

	unsigned int compile(GLenum type, const char* src)
	{
		unsigned int shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);
		int ok = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			char log[1024];
			glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
			throw std::runtime_error(std::string("shader compile failed: ") + log);
		}
		return shader;
	}

	const float fullUV[4] = { 0, 0, 1, 1 };

	// DxLib の既定フォントの代わり。Windows に入っている英字フォントを使う(日本語は未対応)
	const char* fontCandidates[] = {
		"C:/Windows/Fonts/segoeui.ttf",
		"C:/Windows/Fonts/arial.ttf",
	};
}

GlDrawer::GlDrawer(GlfwWindow& window) : window(window) {}

GlDrawer::~GlDrawer()
{
	if (!program) return; // init 前
	for (auto& [size, font] : fonts) glDeleteTextures(1, &font->texture);
	for (auto& m : meshes)
	{
		glDeleteBuffers(1, &m.vbo);
		glDeleteBuffers(1, &m.ebo);
		glDeleteVertexArrays(1, &m.vao);
	}
	glDeleteTextures(1, &whiteTexture);
	for (auto& t : targets)
	{
		glDeleteFramebuffers(1, &t.fbo);
		glDeleteTextures(1, &t.color);
		glDeleteRenderbuffers(1, &t.depth);
	}
	glDeleteProgram(program3D);
	if (!textures.empty()) glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

void GlDrawer::init()
{
	if (!gladLoadGL(glfwGetProcAddress)) throw std::runtime_error("gladLoadGL failed");

	program = link(compile(GL_VERTEX_SHADER, vertexSrc), compile(GL_FRAGMENT_SHADER, fragmentSrc));
	program3D = link(compile(GL_VERTEX_SHADER, vertex3DSrc), compile(GL_FRAGMENT_SHADER, fragment3DSrc));
	loc3Model = glGetUniformLocation(program3D, "uModel");
	loc3ViewProj = glGetUniformLocation(program3D, "uViewProj");
	loc3NormalMat = glGetUniformLocation(program3D, "uNormalMat");
	loc3Color = glGetUniformLocation(program3D, "uColor");
	loc3LightDir = glGetUniformLocation(program3D, "uLightDir");
	loc3LightColor = glGetUniformLocation(program3D, "uLightColor");
	loc3Ambient = glGetUniformLocation(program3D, "uAmbient");

	//テクスチャの無いメッシュ用の白 1x1
	const unsigned char white[4] = { 255, 255, 255, 255 };
	glGenTextures(1, &whiteTexture);
	glBindTexture(GL_TEXTURE_2D, whiteTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	locRect = glGetUniformLocation(program, "uRect");
	locAngle = glGetUniformLocation(program, "uAngle");
	locScreen = glGetUniformLocation(program, "uScreen");
	locUV = glGetUniformLocation(program, "uUV");
	locColor = glGetUniformLocation(program, "uColor");
	locMode = glGetUniformLocation(program, "uMode");

	const float quad[] = { 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

void GlDrawer::clearScreen(float r, float g, float b)
{
	int fbw, fbh;
	if (currentTarget >= 0)
	{
		fbw = targets[currentTarget].width;
		fbh = targets[currentTarget].height;
	}
	else glfwGetFramebufferSize(window.handle(), &fbw, &fbh);
	glViewport(0, 0, fbw, fbh);
	glClearColor(r, g, b, 1);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	begin2D();
}

void GlDrawer::begin2D()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glUseProgram(program);
	glBindVertexArray(vao);
	glUniform2f(locScreen, static_cast<float>(screenWidth()), static_cast<float>(screenHeight()));
	glActiveTexture(GL_TEXTURE0);
}

ImageResource GlDrawer::createTexture(const unsigned char* fileData, size_t size)
{
	int w, h, channels;
	unsigned char* pixels = stbi_load_from_memory(fileData, static_cast<int>(size), &w, &h, &channels, 4);
	if (!pixels) return {};

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	// DxLib の既定(DX_DRAWMODE_NEAREST)に合わせてドット絵がぼやけないようにする
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	stbi_image_free(pixels);

	textures.push_back(tex);
	return { static_cast<int>(textures.size() - 1), w, h };
}

void GlDrawer::drawQuad(unsigned int texture, float x, float y, float w, float h, float angle, const float uv[4], int mode)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform4f(locRect, x, y, w, h);
	glUniform1f(locAngle, angle);
	glUniform4f(locUV, uv[0], uv[1], uv[2], uv[3]);
	glUniform4f(locColor, 1, 1, 1, 1);
	glUniform1i(locMode, mode);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GlDrawer::drawImage(int handle, float x, float y, float w, float h, float angle)
{
	if (handle < 0 || handle >= static_cast<int>(textures.size())) return;
	drawQuad(textures[handle], x, y, w, h, angle, fullUV, 0);
}

FontAtlas* GlDrawer::getFont(int size)
{
	if (size <= 0) return nullptr;
	auto it = fonts.find(size);
	if (it != fonts.end()) return it->second.get();

	if (!fontTried)
	{
		fontTried = true;
		for (auto path : fontCandidates)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file) continue;
			fontData.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);
			break;
		}
		if (fontData.empty()) std::cerr << "font not found: text will not be drawn" << std::endl;
	}
	if (fontData.empty()) return nullptr;

	auto atlas = std::make_unique<FontAtlas>();
	std::vector<unsigned char> bitmap;
	for (int dim = 256; dim <= 4096; dim *= 2)
	{
		bitmap.assign(dim * dim, 0);
		if (stbtt_BakeFontBitmap(fontData.data(), fontOffset, static_cast<float>(size), bitmap.data(), dim, dim, 32, 95, atlas->chars) > 0)
		{
			atlas->dim = dim;
			break;
		}
	}
	if (atlas->dim == 0) return nullptr;

	stbtt_fontinfo info;
	stbtt_InitFont(&info, fontData.data(), fontOffset);
	int ascent, descent, gap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &gap);
	atlas->ascent = ascent * stbtt_ScaleForPixelHeight(&info, static_cast<float>(size));

	glGenTextures(1, &atlas->texture);
	glBindTexture(GL_TEXTURE_2D, atlas->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas->dim, atlas->dim, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	auto* result = atlas.get();
	fonts[size] = std::move(atlas);
	return result;
}

void GlDrawer::drawText(const std::string& text, int fontSize, Anchor anchor, float x, float y)
{
	FontAtlas* font = getFont(fontSize);
	if (!font || text.empty()) return;

	float w = 0;
	for (unsigned char c : text)
	{
		if (c >= 32 && c < 127) w += font->chars[c - 32].xadvance;
	}
	float h = static_cast<float>(fontSize);

	switch (anchor)
	{
	case Anchor::TOP_LEFT: break;
	case Anchor::TOP_CENTER: x -= w / 2; break;
	case Anchor::TOP_RIGHT: x -= w; break;
	case Anchor::CENTER_LEFT: y -= h / 2; break;
	case Anchor::CENTER: x -= w / 2; y -= h / 2; break;
	case Anchor::CENTER_RIGHT: x -= w; y -= h / 2; break;
	case Anchor::BOTTOM_LEFT: y -= h; break;
	case Anchor::BOTTOM_CENTER: x -= w / 2; y -= h; break;
	case Anchor::BOTTOM_RIGHT: x -= w; y -= h; break;
	}

	float penX = x;
	float baseline = y + font->ascent;
	for (unsigned char c : text)
	{
		if (c < 32 || c >= 127) continue; // ASCII 以外はまだ描けない
		stbtt_aligned_quad q;
		stbtt_GetBakedQuad(font->chars, font->dim, font->dim, c - 32, &penX, &baseline, &q, 1);
		const float uv[4] = { q.s0, q.t0, q.s1, q.t1 };
		drawQuad(font->texture, q.x0, q.y0, q.x1 - q.x0, q.y1 - q.y0, 0, uv, 1);
	}
}

int GlDrawer::createMesh(const MeshData& mesh)
{
	if (mesh.vertices.empty() || mesh.indices.empty()) return -1;
	GpuMesh m;
	glGenVertexArrays(1, &m.vao);
	glGenBuffers(1, &m.vbo);
	glGenBuffers(1, &m.ebo);
	glBindVertexArray(m.vao);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);
	const GLsizei stride = MeshData::STRIDE * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
	m.indexCount = static_cast<int>(mesh.indices.size());
	meshes.push_back(m);
	glBindVertexArray(vao); //2D 用に戻す
	return static_cast<int>(meshes.size() - 1);
}

void GlDrawer::beginScene3D(const Scene3DParams& params)
{
	float aspect = static_cast<float>(screenWidth()) / static_cast<float>(screenHeight());
	glm::mat4 proj = glm::perspective(glm::radians(params.fov), aspect, params.nearClip, params.farClip);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glUseProgram(program3D);
	glUniformMatrix4fv(loc3ViewProj, 1, GL_FALSE, &(proj * params.view)[0][0]);
	glUniform3fv(loc3LightDir, 1, &params.lightDirection[0]);
	glUniform3fv(loc3LightColor, 1, &params.lightColor[0]);
	glUniform1f(loc3Ambient, params.ambient);
	glActiveTexture(GL_TEXTURE0);
}

void GlDrawer::drawMesh(int mesh, int texture, const glm::mat4& model, const glm::vec3& color)
{
	if (mesh < 0 || mesh >= static_cast<int>(meshes.size())) return;
	glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(loc3Model, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix3fv(loc3NormalMat, 1, GL_FALSE, &normalMat[0][0]);
	glUniform3fv(loc3Color, 1, &color[0]);
	bool hasTexture = texture >= 0 && texture < static_cast<int>(textures.size());
	glBindTexture(GL_TEXTURE_2D, hasTexture ? textures[texture] : whiteTexture);
	glBindVertexArray(meshes[mesh].vao);
	glDrawElements(GL_TRIANGLES, meshes[mesh].indexCount, GL_UNSIGNED_INT, nullptr);
}

void GlDrawer::endScene3D()
{
	begin2D();
}

int GlDrawer::screenWidth() const
{
	return currentTarget >= 0 ? targets[currentTarget].width : window.getWidth();
}

int GlDrawer::screenHeight() const
{
	return currentTarget >= 0 ? targets[currentTarget].height : window.getHeight();
}

int GlDrawer::createRenderTarget(int w, int h)
{
	RenderTarget t;
	t.width = w;
	t.height = h;
	glGenFramebuffers(1, &t.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
	glGenTextures(1, &t.color);
	glBindTexture(GL_TEXTURE_2D, t.color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.color, 0);
	glGenRenderbuffers(1, &t.depth);
	glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t.depth);
	bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_FRAMEBUFFER, currentTarget >= 0 ? targets[currentTarget].fbo : 0);
	if (!ok)
	{
		glDeleteFramebuffers(1, &t.fbo);
		glDeleteTextures(1, &t.color);
		glDeleteRenderbuffers(1, &t.depth);
		return -1;
	}
	targets.push_back(t);
	return static_cast<int>(targets.size() - 1);
}

void GlDrawer::setRenderTarget(int target)
{
	if (target >= static_cast<int>(targets.size())) target = -1;
	currentTarget = target;
	glBindFramebuffer(GL_FRAMEBUFFER, target >= 0 ? targets[target].fbo : 0);
}

uintptr_t GlDrawer::renderTargetTexture(int target)
{
	if (target < 0 || target >= static_cast<int>(targets.size())) return 0;
	return targets[target].color;
}

bool GlDrawer::captureScreen(const std::string& path)
{
	int fbw, fbh;
	if (currentTarget >= 0)
	{
		fbw = targets[currentTarget].width;
		fbh = targets[currentTarget].height;
	}
	else glfwGetFramebufferSize(window.handle(), &fbw, &fbh);
	std::vector<unsigned char> pixels(static_cast<size_t>(fbw) * fbh * 4);
	glReadBuffer(currentTarget >= 0 ? GL_COLOR_ATTACHMENT0 : GL_BACK);
	glReadPixels(0, 0, fbw, fbh, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	stbi_flip_vertically_on_write(1);
	return stbi_write_png(path.c_str(), fbw, fbh, 4, pixels.data(), fbw * 4) != 0;
}

void GlDrawer::present()
{
	glfwSwapBuffers(window.handle());
}
