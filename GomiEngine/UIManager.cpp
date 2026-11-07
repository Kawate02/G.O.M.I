#include "UIManager.h"

void UIManager::initialize()
{
}

UIHandle UIManager::createUI()
{
	uint32_t id;
	if (freeIDs.empty())
	{
		id = static_cast<uint32_t>(generations.size());
		generations.push_back(0);
		uiHandles.resize(generations.size());
		uiList.resize(generations.size());
		uiTypes.resize(generations.size(), UIType::NONE);
	}
	else
	{
		id = freeIDs.front();
		freeIDs.pop();
	}
	uiHandles[id] = { id, generations[id] };
	return { id, generations[id] };
}

UIHandle UIManager::add(std::unique_ptr<UIBase> ui, UIType type)
{
	auto handle = createUI();
	uiHandles[handle.id] = handle;
	uiList[handle.id] = std::move(ui);
	uiTypes[handle.id] = type;
	setTop();
	return handle;
}

void UIManager::remove(UIHandle handle)
{
	if (!alive(handle)) return;
	for (auto& child : uiList[handle.id]->getChildren())
	{
		remove(child);
	}
	generations[handle.id]++;
	uiHandles[handle.id] = { handle.id, generations[handle.id] };
	freeIDs.push(handle.id);
	uiTypes[handle.id] = UIType::NONE;
	inputSort.erase(std::remove(inputSort.begin(), inputSort.end(), handle.id), inputSort.end());
	uiList[handle.id] = nullptr;
	setTop();
}

void UIManager::move(UIHandle handle, Vec2 delta)
{
	if (!alive(handle)) return;
	uiList[handle.id]->move(delta);
	for (auto& child : uiList[handle.id]->getChildren())
	{
		move(child, delta);
	}
}

std::vector<UIHandle> UIManager::getHandles(UIType type)
{
	std::vector<UIHandle> result;
	for (uint32_t id = 0; id < uiTypes.size(); id++)
	{
		if (uiTypes[id] == type && uiList[id] != nullptr)
		{
			result.push_back(uiHandles[id]);
		}
	}
	return result;
}

void UIManager::addChild(UIHandle parent, std::unique_ptr<UIBase> ui, UIType type)
{
	auto handle = add(std::move(ui), type);
	if (alive(parent)) uiList[parent.id]->addChild(handle);
}

bool UIManager::alive(UIHandle handle)
{
	return handle.id < generations.size() && handle.generation == generations[handle.id];
}

void UIManager::setTop()
{
	topLayer = 999;
	for (auto& it : uiList)
	{
		if (it == nullptr) continue;
		if (it->getLayer() > topLayer && it->block())
		{
			topLayer = it->getLayer();
		}
	}
}

UIBase* UIManager::getUI(UIHandle handle)
{
	if (!alive(handle)) return nullptr;
	return uiList[handle.id].get();
}

void UIManager::update(const InputState& inputState, RenderQueue& renderQueue, bool interactive)
{
	for (auto& it : uiList)
	{
		if (!interactive) break;
		if (it == nullptr) continue;
		if (topLayer < 1000)
		{
			it->hitTest(inputState);
		}
		else if (it->getLayer() == topLayer)
		{
			if (it->hitTest(inputState) && it->block())
			{
				break;
			}
		}
	}
	for (auto& ui : uiList)
	{
		if (ui == nullptr) continue;
		ui->update(inputState, renderQueue);
	}
}

void UIManager::clear()
{
	for (auto& ui : uiList)
	{
		if (ui) ui->clear();
	}
	uiList.clear();
	uiTypes.clear();
	uiHandles.clear();
	generations.clear();
	std::queue<uint32_t>().swap(freeIDs);
	inputSort.clear();
}
