#include "UIManager.h"

void UIManager::initialize()
{
}

UIHandle UIManager::createUI()
{
	uint32_t id;
	if (freeIDs.empty())
	{
		id = generations.size();
		generations.push_back(0);
		uiHandles.resize(generations.size());
		uiList.resize(generations.size());
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
	uiTypes.push_back(type);
	idToIndex[handle.id] = static_cast<uint8_t>(uiTypes.size() - 1);
	indexToID[static_cast<uint8_t>(uiTypes.size() - 1)] = handle.id;
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
	uiTypes[idToIndex[handle.id]] = UIType::NONE;
	inputSort.erase(std::remove(inputSort.begin(), inputSort.end(), handle.id), inputSort.end());
	uiList[handle.id] = nullptr;
	setTop();
}

std::vector<UIHandle> UIManager::getHandles(UIType type)
{
	std::vector<UIHandle> result;
	for (uint32_t i = 0; i < uiTypes.size(); i++)
	{
		if (uiTypes[i] == type)
		{
			result.push_back(uiHandles[indexToID[i]]);
		}
	}
	return result;
}

void UIManager::addChild(UIHandle parent, std::unique_ptr<UIBase> ui, UIType type)
{
	auto handle = add(std::move(ui), type);
	uiList[parent.id]->addChild(handle);
}

bool UIManager::alive(UIHandle handle)
{
	if (handle.generation == generations[handle.id]) return true;
	return false;
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

std::unique_ptr<UIBase>& UIManager::getUI(UIHandle handle)
{
	std::unique_ptr<UIBase> result = nullptr;
	if (handle.generation != generations[handle.id]) return result;
	return uiList[handle.id];
}

void UIManager::update(const InputState& inputState, RenderQueue& renderQueue)
{
	for (auto& it : uiList)
	{
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
		ui->clear();
		ui.reset();
	}
	uiList.clear();
}
