#include "EditorApp.h"
#include "Project.h"
#include <iostream>

// G.O.M.I エディタ。起動オプションはゲームと同じ(DebugOptions.h)。--world で最初に開くシーン
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
        Project::getInstance().open(Project::findRoot(options.projectDir));
        EditorApp app(options);
        return app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
}
