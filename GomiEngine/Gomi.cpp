#include <iostream>
#include "Game.h"
#include "DxWindow.h"
#include "DxInput.h"
#include "DxDrawer.h"
#include "DxAudioBackend.h"

int main()
{
    Game game(std::make_unique<DxWindow>(), std::make_unique<DxInput>(), std::make_unique<DxDrawer>(), std::make_unique<DxAudioBackend>());
    game.run();
}