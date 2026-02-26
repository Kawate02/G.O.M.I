#pragma once
#include <memory>
#include "IWindow.h"
#include "IInput.h"
#include "IDrawer.h"
#include "IAudioBackend.h"
#include "SettingManager.h"

#include "WorldManager.h"

#include "Renderer.h"
#include "RenderQueue.h"

#include "AudioPlayer.h"
#include "AudioQueue.h"

#include "EventRequest.h"

class Game 
{
private:
    EventRequest<int> quitRequest{Event::Game_Quit};
    EventRequest<AudioCommand> audioRequest{Event::Audio_Play};
	GameSettings settings;
    std::unique_ptr<IWindow> window;
    std::unique_ptr<IInput> input;
    std::unique_ptr<IDrawer> drawer;
    std::unique_ptr<IAudioBackend> audioBackend;

    InputState inputState;
	Renderer renderer;
	RenderQueue renderQueue;
	AudioPlayer audioPlayer;
	AudioQueue audioQueue;

    WorldManager world;

    const int TERGET_FPS = 60;
    const int TERGET_FRAME_TIME = 1000 / TERGET_FPS;

    int frameCount = 0;
    int lastTime = 0;
public:
    Game(std::unique_ptr<IWindow> w,
        std::unique_ptr<IInput> i,
        std::unique_ptr<IDrawer> d,
        std::unique_ptr<IAudioBackend> a)
        : settings(), window(std::move(w)), input(std::move(i)), drawer(std::move(d)), audioBackend(std::move(a)), world(inputState, renderQueue, audioQueue) {}

    void run();
    void init();
};
