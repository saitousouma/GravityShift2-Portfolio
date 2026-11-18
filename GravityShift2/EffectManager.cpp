#include "EffectManager.h"
#include "DxLib.h"

void EffectManager::Clear()
{
	// 各要素を初期状態へ戻し、再生中フラグもまとめて解除する。
	effects_.fill(Effect{});
}


bool EffectManager::Load()
{
	// 敵撃破エフェクトの画像と、シート上のフレーム情報を設定する。
	enemyDestroyAnimation_.imageHandle = LoadGraph(L"Assets/Images/Effects/enemy-destroy-sheet2.png");

	if (enemyDestroyAnimation_.imageHandle == -1)
	{
		return false;
	}

	enemyDestroyAnimation_.frameCount = 11;
	enemyDestroyAnimation_.frameWidth = 128;
	enemyDestroyAnimation_.frameHeight = 128;
	enemyDestroyAnimation_.framesPerRow = 4;
	enemyDestroyAnimation_.frameDuration = 4;
	enemyDestroyAnimation_.drawScaleX = 0.75f;
	enemyDestroyAnimation_.drawScaleY = 0.75f;

	// ボス撃破、プレイヤー死亡、各ギミック用のアニメーションを順に読み込む。
	bossDestroyAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/boss-destroy-sheet.png");

	if (bossDestroyAnimation_.imageHandle == -1)
	{
		return false;
	}

	bossDestroyAnimation_.frameCount = 12;
	bossDestroyAnimation_.frameWidth = 96;
	bossDestroyAnimation_.frameHeight = 96;
	bossDestroyAnimation_.frameDuration = 4;
	bossDestroyAnimation_.startFrame = 0;

	playerDestroyAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/player-destroy-sheet.png");

	if (playerDestroyAnimation_.imageHandle == -1)
	{
		return false;
	}

	playerDestroyAnimation_.frameCount = 11;
	playerDestroyAnimation_.frameWidth = 64;
	playerDestroyAnimation_.frameHeight = 64;
	playerDestroyAnimation_.frameDuration = 4;
	playerDestroyAnimation_.startFrame = 0;
	playerDestroyAnimation_.drawScaleX = 1.5f;
	playerDestroyAnimation_.drawScaleY = 1.5f;

	gravityBlockLandingAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/gravity-block-landing-sheet.png");

	if (gravityBlockLandingAnimation_.imageHandle == -1)
	{
		return false;
	}

	gravityBlockLandingAnimation_.frameCount = 11;
	gravityBlockLandingAnimation_.frameWidth = 64;
	gravityBlockLandingAnimation_.frameHeight = 64;
	gravityBlockLandingAnimation_.frameDuration = 3;
	gravityBlockLandingAnimation_.startFrame = 0;
	gravityBlockLandingAnimation_.drawScaleX = 1.6f;
	gravityBlockLandingAnimation_.drawScaleY = 1.0f;

	checkpointAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/checkpoint-effect-sheet.png");

	if (checkpointAnimation_.imageHandle == -1)
	{
		return false;
	}

	checkpointAnimation_.frameCount = 29;
	checkpointAnimation_.frameWidth = 128;
	checkpointAnimation_.frameHeight = 128;
	checkpointAnimation_.frameDuration = 2;
	checkpointAnimation_.startFrame = 0;

	gravityReverseAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/gravity-reverse-sheet.png");
	if (gravityReverseAnimation_.imageHandle == -1)
	{
		return false;
	}
	gravityReverseAnimation_.frameCount = 12;
	gravityReverseAnimation_.frameWidth = 64;
	gravityReverseAnimation_.frameHeight = 64;
	gravityReverseAnimation_.frameDuration = 3;
	gravityReverseAnimation_.startFrame = 0;
	gravityReverseAnimation_.drawScaleX = 1.25f;
	gravityReverseAnimation_.drawScaleY = 1.25f;
	// レーザー発射エフェクトは素材調整中のため、現在は読み込まない。
	/*
	laserFireAnimation_.imageHandle =
		LoadGraph(L"Assets/Images/Effects/laser-fire-burst.png");

	if (laserFireAnimation_.imageHandle == -1)
	{
		return false;
	}

	laserFireAnimation_.frameCount = 8;
	laserFireAnimation_.frameWidth = 72;
	laserFireAnimation_.frameHeight = 72;
	laserFireAnimation_.frameDuration = 3;

	laserFireAnimation_.startFrame = 7;

	return true;
	*/

	return true;
}

void EffectManager::PlayEnemyDestroyEffect(const Vector2& position)
{
	// 再生が終わった要素を探して、新しいエフェクトに再利用する。
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, enemyDestroyAnimation_);
			return;
		}
	}
}

void EffectManager::PlayBossDestroyEffect(const Vector2& position)
{
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, bossDestroyAnimation_);
			return;
		}
	}
}


void EffectManager::PlayPlayerDestroyEffect(const Vector2& position)
{
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, playerDestroyAnimation_);
			return;
		}
	}
}

void EffectManager::PlayLaserFireEffect(const Vector2& position)
{
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, laserFireAnimation_);
			return;
		}
	}
}

void EffectManager::PlayGravityBlockLandingEffect(const Vector2& position)
{
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, gravityBlockLandingAnimation_);
			return;
		}
	}
}

void EffectManager::PlayCheckpointEffect(const Vector2& position)
{
	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(position, checkpointAnimation_);
			return;
		}
	}
}

void EffectManager::PlayGravityReverseEffect(
	const Vector2& playerPosition,
	float playerWidth, float playerHeight, bool isFootAtTop)
{
	// 天井側では画像を上下反転し、反転前の足元へ位置を合わせる。
	EffectAnimationData animation = gravityReverseAnimation_;
	animation.flipY = isFootAtTop;
	const float drawWidth = animation.frameWidth * animation.drawScaleX;
	const float drawHeight = animation.frameHeight * animation.drawScaleY;
	const Vector2 effectPosition{
		playerPosition.x + playerWidth * 0.5f - drawWidth * 0.5f,
		isFootAtTop
			? playerPosition.y
			: playerPosition.y + playerHeight - drawHeight
	};

	for (Effect& effect : effects_)
	{
		if (!effect.IsActive())
		{
			effect.Start(effectPosition, animation);
			return;
		}
	}
}

void EffectManager::Update()
{
	// 非アクティブな要素はEffect側で何もせず戻る。
	for (Effect& effect : effects_)
	{
		effect.Update();
	}
}

void EffectManager::Draw() const
{
	for (const Effect& effect : effects_)
	{
		effect.Draw();
	}
}
