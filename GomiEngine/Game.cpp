#include "Game.h"

std::string Game::addWorld(const std::string& name, const StageDefinition& stageDef)
{
	return world.addWorld(name, stageDef);
}

std::string Game::addWorld(const StageDefinition& stageDef)
{
	return world.addWorld(stageDef);
}

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
		int nowTime = GetNowCount(); //�t���[�����[�g����p
		float deltaTime = (nowTime - lastTime) / 1000.0f;
		lastTime = nowTime;

        input->inputStateUpdate();
        inputState = input->getInputState(); //���͏��̒��o

		world.update(deltaTime); //���C������

		if (AudioCommand e; audioRequest.trigger(e)) audioQueue.addCommand({ e.audioID, e.volume });

        renderer.render(*drawer, renderQueue, world.getWorld().getCamera()); //�`�揈��
		audioPlayer.play(*audioBackend, audioQueue); //��������

		if (nowTime - lastTime >= 1000)
		{
			std::cout << "\rFPS:" << frameCount << " " << std::flush;
			frameCount = 0;
			lastTime = nowTime;
		}
		//�t���[�����[�g����
		int frameTime = GetNowCount() - nowTime;
		if (frameTime < TERGET_FRAME_TIME)
		{
			WaitTimer(TERGET_FRAME_TIME - frameTime);
		}
    }
	std::cout << "\nGame End" << std::endl;
}
