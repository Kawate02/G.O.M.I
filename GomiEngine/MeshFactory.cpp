#include "MeshFactory.h"
#include <cmath>

namespace
{
	void pushVertex(MeshData& m, glm::vec3 p, glm::vec3 n, glm::vec2 uv)
	{
		m.vertices.insert(m.vertices.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
	}

	/// <summary>
	/// 法線 n を向いた面を1枚(4頂点・2三角形、表は反時計回り)足す
	/// </summary>
	void pushFace(MeshData& m, glm::vec3 n, glm::vec3 right, glm::vec3 up)
	{
		uint32_t base = static_cast<uint32_t>(m.vertices.size() / MeshData::STRIDE);
		glm::vec3 c = n * 0.5f;
		pushVertex(m, c - right * 0.5f + up * 0.5f, n, { 0, 0 });
		pushVertex(m, c - right * 0.5f - up * 0.5f, n, { 0, 1 });
		pushVertex(m, c + right * 0.5f - up * 0.5f, n, { 1, 1 });
		pushVertex(m, c + right * 0.5f + up * 0.5f, n, { 1, 0 });
		m.indices.insert(m.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
	}
}

MeshData MeshFactory::cube()
{
	MeshData m;
	pushFace(m, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 });   // 手前
	pushFace(m, { 0, 0, -1 }, { -1, 0, 0 }, { 0, 1, 0 }); // 奥
	pushFace(m, { 1, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 });  // 右
	pushFace(m, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 });  // 左
	pushFace(m, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, -1 });  // 上
	pushFace(m, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 });  // 下
	return m;
}

MeshData MeshFactory::plane()
{
	MeshData m;
	pushFace(m, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, -1 });
	//面は中心から 0.5 上にずれているので y = 0 に戻す
	for (size_t i = 1; i < m.vertices.size(); i += MeshData::STRIDE) m.vertices[i] = 0;
	return m;
}

MeshData MeshFactory::sphere(int segments, int rings)
{
	MeshData m;
	const float pi = 3.14159265358979f;
	for (int r = 0; r <= rings; r++)
	{
		float v = static_cast<float>(r) / rings;
		float phi = v * pi;
		for (int s = 0; s <= segments; s++)
		{
			float u = static_cast<float>(s) / segments;
			float theta = u * 2 * pi;
			glm::vec3 n(std::sin(phi) * std::sin(theta), std::cos(phi), std::sin(phi) * std::cos(theta));
			pushVertex(m, n * 0.5f, n, { u, v });
		}
	}
	for (int r = 0; r < rings; r++)
	{
		for (int s = 0; s < segments; s++)
		{
			uint32_t a = r * (segments + 1) + s;
			uint32_t b = a + segments + 1;
			m.indices.insert(m.indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}
	}
	return m;
}

std::optional<MeshData> MeshFactory::builtin(const std::string& id)
{
	if (id == "builtin:cube") return cube();
	if (id == "builtin:plane") return plane();
	if (id == "builtin:sphere") return sphere();
	return std::nullopt;
}

void MeshFactory::computeNormals(MeshData& mesh)
{
	const int S = MeshData::STRIDE;
	size_t count = mesh.vertices.size() / S;
	std::vector<glm::vec3> normals(count, glm::vec3(0));
	for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
	{
		uint32_t ia = mesh.indices[i], ib = mesh.indices[i + 1], ic = mesh.indices[i + 2];
		auto pos = [&](uint32_t k) { return glm::vec3(mesh.vertices[k * S], mesh.vertices[k * S + 1], mesh.vertices[k * S + 2]); };
		glm::vec3 n = glm::cross(pos(ib) - pos(ia), pos(ic) - pos(ia)); //面積で重み付け
		normals[ia] += n;
		normals[ib] += n;
		normals[ic] += n;
	}
	for (size_t k = 0; k < count; k++)
	{
		glm::vec3 n = glm::length(normals[k]) > 0 ? glm::normalize(normals[k]) : glm::vec3(0, 1, 0);
		mesh.vertices[k * S + 3] = n.x;
		mesh.vertices[k * S + 4] = n.y;
		mesh.vertices[k * S + 5] = n.z;
	}
}
