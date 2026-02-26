#include "Prefab.h"
#include "World.h"

Entity Prefab_BackGround(World& world, const SpawnDef& def)
{
	auto& component = world.getComponentManager();
	Entity b = world.createEntity();
	component.addComponent(b, BackGroundComponent({ .asset = "background", .origin = { 0, 0 }, .parallax = { 0, 1 }, .layer = -100, .order = 0, .enable = true, .margin = 0 }));
	Entity m = world.createEntity();
	component.addComponent(m, BackGroundComponent({ .asset = "middleground", .origin = { 0, 0 }, .parallax = { 0.5f, 1 }, .layer = -100, .order = 1, .enable = true, .margin = 0 }));
	Entity f = world.createEntity();
	component.addComponent(f, BackGroundComponent({ .asset = "foreground", .origin = { 0, 0 }, .parallax = { 1, 1 }, .layer = -100, .order = 2, .enable = true, .margin = 0 }));
	Entity a = world.createEntity();
	component.addComponent(a, BackGroundComponent({ .asset = "ag", .origin = { 0, 0 }, .parallax = { 2, 1 }, .layer = 100, .order = 0, .enable = true, .margin = 0 }));

	return b;
}