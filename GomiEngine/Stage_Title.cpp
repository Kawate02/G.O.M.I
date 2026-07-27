#include "Stage_Title.h"
#include "UIPrefab_Title.h"

const StageDefinition Stage_Title = {
	"Title",
	{
		System<RendererSystem>(),
	},
	{

	},
	{
		{ &UIPrefab_Title, { 0.0f, 0.0f } }
	}
};
