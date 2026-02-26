#pragma once
#include "ISystem.h"
#include "Entity.h"

struct CameraRect
{
	int left;
	int top;
	int right;
	int bottom;
};

class OffScreenSystem : public ISystem
{
private:
	CameraRect cameraRect;
	CameraRect marfin = { 50, 50, 50, 50 };
	void despawn(World& world, Entity e);
	void clamp(World& world, Entity e);
	void wrap(World& world, Entity e);
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};