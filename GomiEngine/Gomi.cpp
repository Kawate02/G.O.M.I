#include <iostream>
#include "Game.h"
#include "DxWindow.h"
#include "DxInput.h"
#include "DxDrawer.h"
#include "DxAudioBackend.h"
#include "Stage_Title.h"
#include "Stage_Field.h"
#include "Stage_Result.h"

int main()
{
    Game game(std::make_unique<DxWindow>(), std::make_unique<DxInput>(), std::make_unique<DxDrawer>(), std::make_unique<DxAudioBackend>());
    game.addWorld(Stage_Title);
    game.addWorld(Stage_Field);
    game.addWorld(Stage_Result);
    game.run();
}