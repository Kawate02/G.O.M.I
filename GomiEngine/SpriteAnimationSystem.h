#pragma once
#include "ISystem.h"
#include "SpriteAnimationComponent.h"

#include <string>

/// <summary>
/// SpriteAnimation の time を進め、今のコマを Renderer の path と uv に書く(描画の命令は作らない。RendererSystem より前に置く)
/// </summary>
class SpriteAnimationSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};

/// <summary>
/// entity の SpriteAnimation で name のクリップを流す(ゲームのコードからはこれと isAnimationFinished だけ使う)。
/// 同じ名前を流している時は何もしない(毎フレーム呼んでよい)。restart が true なら同じ名前でも最初から流す。
/// 知らない名前・SpriteAnimation が無い時はエラーを1回ログに出して、今のままにする
/// </summary>
void playAnimation(World& world, Entity entity, const std::string& name, bool restart = false);

/// <summary>
/// ループしないクリップを最後まで流し終えたら true。ループするクリップ・何も流していない時は false
/// </summary>
bool isAnimationFinished(World& world, Entity entity);
