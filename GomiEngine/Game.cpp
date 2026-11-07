#include "Game.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "SceneLoader.h"
#include <chrono>
#include <thread>

void Game::init()
{
	registerBuiltinComponents();
	registerBuiltinSystems();
	auto& sm = SettingManager::getInstance();
	window->init(sm.getValue<int>("windowWidth"), sm.getValue<int>("windowHeight"), "Gomi", sm.getValue<bool>("fullscreen"));
	drawer->init();
	audioBackend->init();
    renderer.init();
    world.initialize(debug.startWorld);
}

void Game::run()
{
	using clock = std::chrono::steady_clock;
	const auto targetFrameTime = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / TARGET_FPS));

	init();
	auto lastTime = clock::now();
	auto fpsTimer = lastTime;
	auto nextFrame = lastTime;
	int fpsCount = 0;
	int totalFrames = 0;
    while (window->processMessage())
    {
		if (quitRequest.trigger()) break;
		totalFrames++;
		fpsCount++;
		auto nowTime = clock::now();
		float deltaTime = std::chrono::duration<float>(nowTime - lastTime).count();
		lastTime = nowTime;
		if (debug.fixedDeltaTime > 0) deltaTime = debug.fixedDeltaTime;

        input->inputStateUpdate();
        inputState = input->getInputState(); //入力情報の抽出
		debug.apply(totalFrames, inputState);
		for (auto& path : debug.sceneSavesAt(totalFrames))
		{
			AssetDatabase::getInstance().writeJson(path, SceneLoader::save(world.getWorld()));
		}

		world.update(deltaTime); //メイン処理

		if (AudioCommand e; audioRequest.trigger(e)) audioQueue.addCommand({ e.audioID, e.volume });

        renderer.render(*drawer, renderQueue, world.getWorld().getCamera()); //描画処理
		for (auto& path : debug.screenshotsAt(totalFrames))
		{
			if (!drawer->captureScreen(path)) std::cerr << "screenshot failed: " << path << std::endl;
		}
		drawer->present();
		audioPlayer.play(*audioBackend, audioQueue); //音声処理

		if (nowTime - fpsTimer >= std::chrono::seconds(1))
		{
			std::cout << "\rFPS:" << fpsCount << " " << std::flush;
			fpsCount = 0;
			fpsTimer = nowTime;
		}
		EventBus::getInstance().cleanup();
		if (debug.shouldQuit(totalFrames)) break;

		//フレームレート調整
		//予定時刻を積み上げて寝過ごしを次のフレームで取り返す。最後の1msは sleep の粒度が粗いので空回しで待つ
		nextFrame += targetFrameTime;
		auto now = clock::now();
		if (nextFrame < now - targetFrameTime) nextFrame = now; //大きく遅れたら取り返さずに仕切り直す
		if (nextFrame - now > std::chrono::milliseconds(1)) std::this_thread::sleep_until(nextFrame - std::chrono::milliseconds(1));
		while (clock::now() < nextFrame) std::this_thread::yield();
    }
	std::cout << "\nGame End" << std::endl;
}
