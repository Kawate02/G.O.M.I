#pragma once
#include "EnumNames.h"
#include "UIBase.h"
#include "RenderQueue.h"
#include "Utilities.h"

enum class UIType : uint8_t
{
	NONE = 0,
	RESULT
};
GOMI_ENUM_NAMES(UIType,
	GOMI_ENUM_ENTRY(UIType, NONE),
	GOMI_ENUM_ENTRY(UIType, RESULT))

class UIManager
{
private:
	std::vector<std::unique_ptr<UIBase>> uiList;
	int topLayer = 1000;
	std::vector<UIType> uiTypes; //添字 = UIHandle の id

	std::vector<UIHandle> uiHandles;
	std::vector<uint32_t> generations;
	std::queue<uint32_t> freeIDs;

	std::vector<uint32_t> inputSort;
public:
	void initialize();
	UIHandle createUI();
	UIHandle add(std::unique_ptr<UIBase> ui, UIType type = UIType::NONE);
	void remove(UIHandle handle);
	void move(UIHandle handle, Vec2 delta);
	std::vector<UIHandle> getHandles(UIType type);
	void addChild(UIHandle parent, std::unique_ptr<UIBase> ui, UIType type = UIType::NONE);
	bool alive(UIHandle handle);
	void setTop();
	/// <summary>
	/// 無効なハンドルなら nullptr
	/// </summary>
	UIBase* getUI(UIHandle handle);
	void update(const InputState& inputState, RenderQueue& renderQueue);
	void clear();
};