#pragma once
#include "UIBase.h"
#include "RenderQueue.h"
#include "Utilities.h"

enum class UIType : uint8_t
{
	NONE = 0,
	RESULT
};
class UIManager
{
private:
	std::vector<std::unique_ptr<UIBase>> uiList;
	int topLayer = 1000;
	std::vector<UIType> uiTypes;
	std::map<uint32_t, uint8_t> idToIndex;
	std::map<uint8_t, uint32_t> indexToID;

	std::vector<UIHandle> uiHandles;
	std::vector<uint8_t> generations;
	std::queue<uint8_t> freeIDs;

	std::vector<uint32_t> inputSort;
public:
	void initialize();
	UIHandle createUI();
	UIHandle add(std::unique_ptr<UIBase> ui, UIType type = UIType::NONE);
	void remove(UIHandle handle);
	std::vector<UIHandle> getHandles(UIType type);
	void addChild(UIHandle parent, std::unique_ptr<UIBase> ui, UIType type = UIType::NONE);
	bool alive(UIHandle handle);
	void setTop();
	std::unique_ptr<UIBase>& getUI(UIHandle handle);
	void update(const InputState& inputState, RenderQueue& renderQueue);
	void clear();
};