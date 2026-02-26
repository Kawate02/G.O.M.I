#include "DxWindow.h"

#include "EventBus.h"

void DxWindow::init(int width, int height, const char* title, bool fullscreen)
{
    ChangeWindowMode(fullscreen ? FALSE : TRUE);
    SetGraphMode(width, height, 32);
    SetMainWindowText(title);
    SetWaitVSyncFlag(FALSE);
    DxLib_Init();
}
