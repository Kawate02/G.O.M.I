#pragma once
#include <optional>
#include "RenderCommand.h"

/// <summary>
/// 1フレーム分の描画命令。System・UI が積み、Renderer がまとめて描いて空にする
/// </summary>
class RenderQueue
{
private:
	std::vector<RenderCommand> commands;
	std::vector<MeshRenderCommand> meshes;
	std::optional<Camera3DView> camera3D;
	LightData light;
public:
	void addCommand(const RenderCommand& command)
	{
		commands.push_back(command);
	}
	const std::vector<RenderCommand>& getCommands() const
	{
		return commands;
	}
	void addMesh(const MeshRenderCommand& mesh)
	{
		meshes.push_back(mesh);
	}
	const std::vector<MeshRenderCommand>& getMeshes() const
	{
		return meshes;
	}
	inline void setCamera3D(const Camera3DView& camera) { camera3D = camera; }
	inline const std::optional<Camera3DView>& getCamera3D() const { return camera3D; }
	inline void setLight(const LightData& data) { light = data; }
	inline const LightData& getLight() const { return light; }

	void clear()
	{
		commands.clear();
		meshes.clear();
		camera3D.reset();
		light = LightData();
	}
};
