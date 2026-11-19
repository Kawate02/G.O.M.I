#include "GameCode.h"
#include "PlayerStatus.h"

GOMI_REGISTER
{
	resources.add<PlayerStatus>("PlayerStatus")
		.field("hp", &PlayerStatus::hp)
		.field("items", &PlayerStatus::items);
}
