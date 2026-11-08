#include <iostream>
#include "Game.h"
#include "AssetDatabase.h"
#include "GlfwWindow.h"
#include "GlfwInput.h"
#include "GlDrawer.h"
#include "MiniaudioBackend.h"

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
        AssetDatabase::getInstance().setRoot(AssetDatabase::findRoot(options.assetDir));
        auto window = std::make_unique<GlfwWindow>();
        GlfwWindow& w = *window;
        Game game(std::move(window), std::make_unique<GlfwInput>(w), std::make_unique<GlDrawer>(w), std::make_unique<MiniaudioBackend>());
        game.setDebugOptions(options);
        return game.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
}
