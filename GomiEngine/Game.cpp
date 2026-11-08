#include "Game.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "SceneLoader.h"
#include "Random.h"
#include <chrono>
#include <cmath>
#include <filesystem>
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

void Game::startReplay()
{
	namespace fs = std::filesystem;
	if (!debug.replayPath.empty())
	{
		Replay replay = Replay::load(debug.replayPath);
		replayName = fs::path(debug.replayPath).stem().string();
		debug.startWorld = replay.scene;
		debug.fixedDeltaTime = replay.deltaTime();
		debug.maxFrames = replay.length;
		Random::seed(replay.seed);
		fs::path out = debug.replayOut.empty() ? fs::temp_directory_path() / "gomi-replay" / replayName : fs::path(debug.replayOut);
		replayCheck.emplace(Replay::expectedDir(debug.replayPath), out, debug.replayUpdate ? ReplayCheck::Mode::Update : ReplayCheck::Mode::Check);
		replayPlayer.emplace(std::move(replay));
	}
	else if (!debug.recordPath.empty())
	{
		if (debug.fixedDeltaTime <= 0) debug.fixedDeltaTime = 1.0f / TARGET_FPS;
		const uint32_t seed = 1;
		Random::seed(seed);
		recorder.emplace(debug.startWorld, static_cast<int>(std::lround(1.0f / debug.fixedDeltaTime)), seed);
	}
}

int Game::finishReplay(int lastFrame)
{
	if (recorder)
	{
		Replay replay = recorder->finish(lastFrame);
		replay.scene = firstScene;
		replay.save(debug.recordPath);
		std::cout << "\nrecorded " << lastFrame << " frames, " << replay.events.size() << " input changes, " << replay.checkpoints.size() << " checkpoints: " << debug.recordPath << std::endl;
		std::cout << "run with --replay " << debug.recordPath << " --replay-update to save the expected screens" << std::endl;
	}
	if (replayCheck)
	{
		std::cout << "\n";
		for (auto& line : replayCheck->report(replayPlayer->getReplay(), replayName)) std::cout << line << std::endl;
		return replayCheck->passed(replayPlayer->getReplay()) ? 0 : 1;
	}
	return 0;
}

int Game::run()
{
	using clock = std::chrono::steady_clock;
	const auto targetFrameTime = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / TARGET_FPS));

	startReplay();
	init();
	firstScene = world.getCurrentScene();
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
		if (replayPlayer) replayPlayer->apply(totalFrames, inputState);
		if (recorder) recorder->record(totalFrames, inputState);
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
		if (replayCheck && replayPlayer->getReplay().isCheckpoint(totalFrames))
		{
			Json state = { { "scene", world.getCurrentScene() }, { "frame", totalFrames }, { "world", SceneLoader::save(world.getWorld()) } };
			replayCheck->checkpoint(totalFrames, state, [&](const std::string& path) { return drawer->captureScreen(path); });
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
		if (debug.noWait) continue;

		//フレームレート調整
		//予定時刻を積み上げて寝過ごしを次のフレームで取り返す。最後の1msは sleep の粒度が粗いので空回しで待つ
		nextFrame += targetFrameTime;
		auto now = clock::now();
		if (nextFrame < now - targetFrameTime) nextFrame = now; //大きく遅れたら取り返さずに仕切り直す
		if (nextFrame - now > std::chrono::milliseconds(1)) std::this_thread::sleep_until(nextFrame - std::chrono::milliseconds(1));
		while (clock::now() < nextFrame) std::this_thread::yield();
    }
	std::cout << "\nGame End" << std::endl;
	return finishReplay(totalFrames);
}
