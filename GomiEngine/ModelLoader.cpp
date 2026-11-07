#include "ModelLoader.h"
#include "MeshFactory.h"

#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>

namespace
{
	const cgltf_accessor* findAttribute(const cgltf_primitive& prim, cgltf_attribute_type type)
	{
		for (cgltf_size i = 0; i < prim.attributes_count; i++)
		{
			if (prim.attributes[i].type == type && prim.attributes[i].index == 0) return prim.attributes[i].data;
		}
		return nullptr;
	}

	void appendPrimitive(MeshData& out, const cgltf_primitive& prim, const glm::mat4& world, bool& missingNormals)
	{
		if (prim.type != cgltf_primitive_type_triangles) return;
		const cgltf_accessor* pos = findAttribute(prim, cgltf_attribute_type_position);
		if (!pos) return;
		const cgltf_accessor* nrm = findAttribute(prim, cgltf_attribute_type_normal);
		const cgltf_accessor* uv = findAttribute(prim, cgltf_attribute_type_texcoord);
		if (!nrm) missingNormals = true;

		glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(world)));
		uint32_t base = static_cast<uint32_t>(out.vertices.size() / MeshData::STRIDE);
		for (cgltf_size i = 0; i < pos->count; i++)
		{
			float p[3] = { 0, 0, 0 }, n[3] = { 0, 1, 0 }, t[2] = { 0, 0 };
			cgltf_accessor_read_float(pos, i, p, 3);
			if (nrm) cgltf_accessor_read_float(nrm, i, n, 3);
			if (uv) cgltf_accessor_read_float(uv, i, t, 2);
			glm::vec3 wp = glm::vec3(world * glm::vec4(p[0], p[1], p[2], 1.0f));
			glm::vec3 wn = normalMat * glm::vec3(n[0], n[1], n[2]);
			if (glm::length(wn) > 0) wn = glm::normalize(wn);
			out.vertices.insert(out.vertices.end(), { wp.x, wp.y, wp.z, wn.x, wn.y, wn.z, t[0], t[1] });
		}
		if (prim.indices)
		{
			for (cgltf_size i = 0; i < prim.indices->count; i++)
			{
				out.indices.push_back(base + static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i)));
			}
		}
		else
		{
			for (cgltf_size i = 0; i < pos->count; i++) out.indices.push_back(base + static_cast<uint32_t>(i));
		}
		//ノードの拡大が負(鏡写し)だと三角形の表裏が逆になるので並びを戻す
		if (glm::determinant(glm::mat3(world)) < 0)
		{
			for (size_t i = out.indices.size() - (prim.indices ? prim.indices->count : pos->count); i + 2 < out.indices.size(); i += 3)
			{
				std::swap(out.indices[i + 1], out.indices[i + 2]);
			}
		}
	}

	void appendNode(MeshData& out, const cgltf_node* node, bool& missingNormals)
	{
		if (node->mesh)
		{
			float m[16];
			cgltf_node_transform_world(node, m);
			glm::mat4 world = glm::make_mat4(m);
			for (cgltf_size i = 0; i < node->mesh->primitives_count; i++)
			{
				appendPrimitive(out, node->mesh->primitives[i], world, missingNormals);
			}
		}
		for (cgltf_size i = 0; i < node->children_count; i++)
		{
			appendNode(out, node->children[i], missingNormals);
		}
	}
}

std::optional<MeshData> ModelLoader::loadGltf(const std::filesystem::path& path, std::string& error)
{
	cgltf_options options = {};
	cgltf_data* data = nullptr;
	std::string file = path.string();
	if (cgltf_parse_file(&options, file.c_str(), &data) != cgltf_result_success)
	{
		error = "cannot parse glTF";
		return std::nullopt;
	}
	if (cgltf_load_buffers(&options, data, file.c_str()) != cgltf_result_success || cgltf_validate(data) != cgltf_result_success)
	{
		cgltf_free(data);
		error = "broken glTF (buffers / validation)";
		return std::nullopt;
	}

	MeshData mesh;
	bool missingNormals = false;
	const cgltf_scene* scene = data->scene ? data->scene : (data->scenes_count > 0 ? &data->scenes[0] : nullptr);
	if (scene)
	{
		for (cgltf_size i = 0; i < scene->nodes_count; i++) appendNode(mesh, scene->nodes[i], missingNormals);
	}
	else
	{
		for (cgltf_size i = 0; i < data->nodes_count; i++)
		{
			if (!data->nodes[i].parent) appendNode(mesh, &data->nodes[i], missingNormals);
		}
	}
	cgltf_free(data);

	if (mesh.indices.empty())
	{
		error = "no triangle meshes";
		return std::nullopt;
	}
	if (missingNormals) MeshFactory::computeNormals(mesh);
	return mesh;
}
