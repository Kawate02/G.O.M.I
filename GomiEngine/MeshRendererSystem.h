#pragma once
#include "ISystem.h"

/// <summary>
/// 3D の描画命令を積む: 使うカメラ(Camera3D)、光(Light)、メッシュ(MeshRenderer)
/// </summary>
class MeshRendererSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
	bool runsInEditMode() const override { return true; }
};
