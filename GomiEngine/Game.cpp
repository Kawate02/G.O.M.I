#include "Game.h"

void Game::init()
{
	window->init(SettingManager::getInstance().getValue<int>("windowWidth"), SettingManager::getInstance().getValue<int>("windowHeight"), "Gomi", SettingManager::getInstance().getValue<bool>("fullscreen"));
	drawer->init();
    renderer.init();
    world.initialize();
}
void Game::run()
{
	init();
	int lastTime = GetNowCount();
    while (window->processMessage())
    {
		if (quitRequest.trigger()) break;
		frameCount++;
		int nowTime = GetNowCount(); //フレームレート制御用
		float deltaTime = (nowTime - lastTime) / 1000.0f;
		lastTime = nowTime;

        input->inputStateUpdate();
        inputState = input->getInputState(); //入力情報の抽出

		world.update(deltaTime); //メイン処理

		if (AudioCommand e; audioRequest.trigger(e)) audioQueue.addCommand({ e.audioID, e.volume });

        renderer.render(*drawer, renderQueue, world.getWorld().getCamera()); //描画処理
		audioPlayer.play(*audioBackend, audioQueue); //音声処理

		if (nowTime - lastTime >= 1000)
		{
			std::cout << "\rFPS:" << frameCount << " " << std::flush;
			frameCount = 0;
			lastTime = nowTime;
		}
		//フレームレート制御
		int frameTime = GetNowCount() - nowTime;
		if (frameTime < TERGET_FRAME_TIME)
		{
			WaitTimer(TERGET_FRAME_TIME - frameTime);
		}
    }
	std::cout << "\nGame End" << std::endl;
}
