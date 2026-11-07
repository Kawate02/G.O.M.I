#include <iostream>
#include "Game.h"
#include "GlfwWindow.h"
#include "GlfwInput.h"
#include "GlDrawer.h"
#include "MiniaudioBackend.h"
#include "Stage_Title.h"
#include "Stage_Field.h"
#include "Stage_Result.h"

int main(int argc, char** argv)
{
    DebugOptions options;
    if (auto error = options.parse(argc, argv))
    {
        std::cerr << *error << std::endl;
        return 1;
    }

    try
    {
        auto window = std::make_unique<GlfwWindow>();
        GlfwWindow& w = *window;
        Game game(std::move(window), std::make_unique<GlfwInput>(w), std::make_unique<GlDrawer>(w), std::make_unique<MiniaudioBackend>());
        game.setDebugOptions(options);
        game.addWorld(Stage_Title);
        game.addWorld(Stage_Field);
        game.addWorld(Stage_Result);
        game.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
}
