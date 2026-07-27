#include "Prefab_BackGround.h"
#include "ComponentType.h"

const PrefabDefinition Prefab_BackGround = {
	{
		{ { Component(BackGroundComponent({ .asset = "background", .origin = { 0, 0 }, .parallax = { 0, 1 }, .layer = -100, .order = 0, .enable = true, .margin = 0 })) } },
		{ { Component(BackGroundComponent({ .asset = "middleground", .origin = { 0, 0 }, .parallax = { 0.5f, 1 }, .layer = -100, .order = 1, .enable = true, .margin = 0 })) } },
		{ { Component(BackGroundComponent({ .asset = "foreground", .origin = { 0, 0 }, .parallax = { 1, 1 }, .layer = -100, .order = 2, .enable = true, .margin = 0 })) } },
		{ { Component(BackGroundComponent({ .asset = "ag", .origin = { 0, 0 }, .parallax = { 2, 1 }, .layer = 100, .order = 0, .enable = true, .margin = 0 })) } },
	}
};
