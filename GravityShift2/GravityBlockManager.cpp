#include "GravityBlockManager.h"

#include "EffectManager.h"
#include "Bullet.h"
#include "BulletTerrainCollision.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "Player.h"
#include "StageData.h"
#include "DxLib.h"

// 重力ブロック画像を読み込む。
bool GravityBlockManager::Load()
{
	imageHandle_ = LoadGraph(L"Assets/Images/gravity-block.png");

	// 画像が無くても矩形描画で続けられるので、初期化失敗にはしない。
	return true;
}

// 読み込んだ画像を解放する。
void GravityBlockManager::Release()
{
	if (imageHandle_ == -1)
	{
		return;
	}

	DeleteGraph(imageHandle_);
	imageHandle_ = -1;
}

// 現在のステージ情報に合わせてブロックを配置する。
void GravityBlockManager::Initialize(const StageData& currentStage)
{
	Clear();

	for (int i = 0; i < currentStage.gravityBlockSpawnCount; ++i)
	{
		const GravityBlockSpawnData& spawnData =
			currentStage.gravityBlockSpawns[i];
		const GravityDirection gravityDirection =
			spawnData.surface == GravityBlockSpawnSurface::Ceiling
			? GravityDirection::Up
			: GravityDirection::Down;

		AddBlock(spawnData.position, gravityDirection, spawnData.motion);
	}
}

// すべてのブロックを更新する。
void GravityBlockManager::Update(
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	for (int i = 0; i < blockCount_; ++i)
	{
		blocks_[i].Update(gravitySystem, currentStage);
	}
}

// カメラ範囲と少しでも重なるブロックへ重力反転を通知する。
void GravityBlockManager::OnGravityReversed(
	float cameraX,
	float cameraY,
	float cameraWidth,
	float cameraHeight)
{
	const float cameraRight = cameraX + cameraWidth;
	const float cameraBottom = cameraY + cameraHeight;

	for (int i = 0; i < blockCount_; ++i)
	{
		GravityBlock& block = blocks_[i];
		if (!block.IsActive())
		{
			continue;
		}

		const Vector2& blockPosition = block.GetPosition();
		const float blockRight = blockPosition.x + block.GetWidth();
		const float blockBottom = blockPosition.y + block.GetHeight();
		const bool overlapsCamera =
			blockPosition.x < cameraRight &&
			blockRight > cameraX &&
			blockPosition.y < cameraBottom &&
			blockBottom > cameraY;
		if (!overlapsCamera)
		{
			continue;
		}

		block.OnGravityReversed();
	}
}

// すべてのブロックを描画する。
void GravityBlockManager::Draw() const
{
	for (int i = 0; i < blockCount_; ++i)
	{
		blocks_[i].Draw();
	}
}

// 着地したブロックごとに、接地面付近へエフェクトを出す。
// 1つでも再生したらtrueを返し、画面揺れなどのきっかけに使う。
bool GravityBlockManager::PlayLandingEffects(EffectManager& effectManager) const
{
	bool playedLandingEffect = false;

	for (int i = 0; i < blockCount_; ++i)
	{
		const GravityBlock& block = blocks_[i];

		if (!block.WasLandedThisFrame())
		{
			continue;
		}

		const float effectDrawWidth = 64.0f * 1.6f;
		const float effectDrawHeight = 64.0f;
		const float effectPositionOffsetX = 4.0f;

		Vector2 effectPosition{};
		effectPosition.x =
			block.GetPosition().x + block.GetWidth() * 0.5f - effectDrawWidth * 0.5f + effectPositionOffsetX;

		if (block.GetGravityDirection() == GravityDirection::Down)
		{
			effectPosition.y = block.GetPosition().y + block.GetHeight() - effectDrawHeight * 0.5f;
		}
		else
		{
			effectPosition.y = block.GetPosition().y - effectDrawHeight * 0.5f;
		}

		effectManager.PlayGravityBlockLandingEffect(effectPosition);
		playedLandingEffect = true;
	}

	return playedLandingEffect;
}

// すべてのブロックを調べ、横からぶつかったブロックがあればプレイヤーを押し戻す。
bool GravityBlockManager::ResolvePlayerHorizontalCollision(Player& player) const
{
	for (int i = 0; i < blockCount_; ++i)
	{
		if (player.ResolveBlockHorizontalCollision(blocks_[i]))
		{
			return true;
		}
	}

	return false;
}

// すべてのブロックを調べ、プレイヤーが乗れるブロックがあれば接地させる。
bool GravityBlockManager::ResolvePlayerVerticalCollision(
	Player& player,
	const GravitySystem& gravitySystem) const
{
	for (int i = 0; i < blockCount_; ++i)
	{
		if (player.ResolveBlockVerticalCollision(blocks_[i], gravitySystem))
		{
			return true;
		}
	}

	return false;
}

// 通常型は落下中だけ、往復型は動作中か待機中かにかかわらず接触ダメージを与える。
bool GravityBlockManager::CheckPlayerCollision(
	const Player& player,
	float& damageSourceCenterX) const
{
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();


	for (int i = 0; i < blockCount_; ++i)
	{
		const GravityBlock& block = blocks_[i];

		if (!block.IsDamaging())
		{
			continue;
		}

		const float blockLeft = block.GetPosition().x;
		const float blockTop = block.GetPosition().y;
		const float blockRight = blockLeft + block.GetWidth();
		const float blockBottom = blockTop + block.GetHeight();

		const bool isHit =
			playerLeft < blockRight &&
			playerRight > blockLeft &&
			playerTop < blockBottom &&
			playerBottom > blockTop;

		if (isHit)
		{
			damageSourceCenterX =
				block.GetPosition().x + block.GetWidth() * 0.5f;
			return true;
		}
	}

	return false;
}

// 敵弾が有効な重力ブロックに当たっているか調べる。
bool GravityBlockManager::CheckEnemyBulletCollision(const EnemyBullet& bullet) const
{
	if (!bullet.IsActive())
	{
		return false;
	}

	const float bulletLeft = bullet.GetPosition().x;
	const float bulletTop = bullet.GetPosition().y;
	const float bulletRight = bulletLeft + bullet.GetWidth();
	const float bulletBottom = bulletTop + bullet.GetHeight();

	for (int i = 0; i < blockCount_; ++i)
	{
		const GravityBlock& block = blocks_[i];

		if (!block.IsActive())
		{
			continue;
		}

		const float blockLeft = block.GetPosition().x;
		const float blockTop = block.GetPosition().y;
		const float blockRight = blockLeft + block.GetWidth();
		const float blockBottom = blockTop + block.GetHeight();

		const bool isColliding =
			bulletLeft < blockRight &&
			bulletRight > blockLeft &&
			bulletTop < blockBottom &&
			bulletBottom > blockTop;

		if (isColliding)
		{
			return true;
		}
	}

	return false;
}

// 弾の移動前後の横幅と、現在有効な重力ブロックの矩形を比較する。
bool GravityBlockManager::CheckPlayerBulletCollision(
	const Bullet& bullet, const Vector2& previousPosition) const
{
	if (!bullet.IsActive())
	{
		return false;
	}

	for (int i = 0; i < blockCount_; ++i)
	{
		const GravityBlock& block = blocks_[i];
		if (!block.IsActive())
		{
			continue;
		}

		if (BulletTerrainCollision::IntersectsRect(
			previousPosition, bullet.GetPosition(),
			bullet.GetWidth(), bullet.GetHeight(),
			block.GetPosition(), block.GetWidth(), block.GetHeight()))
		{
			return true;
		}
	}

	return false;
}
// 落下中のブロックと敵の当たり判定を行う。
void GravityBlockManager::CheckEnemyCollisions(
	EnemyManager& enemyManager,
	EffectManager& effectManager,
	ScoreManager& scoreManager) const
{
	for (int i = 0; i < blockCount_; ++i)
	{
		enemyManager.CheckGravityBlockCollisions(
			blocks_[i],
			effectManager,
			scoreManager
		);
	}
}

// 登録済みのブロックをすべて消す。
void GravityBlockManager::Clear()
{
	for (GravityBlock& block : blocks_)
	{
		block.Deactivate();
	}

	blockCount_ = 0;
}

// ブロックを1つ追加する。
void GravityBlockManager::AddBlock(
	const Vector2& position,
	GravityDirection gravityDirection,
	GravityBlockMotion motion)
{
	if (blockCount_ >= MaxBlocks)
	{
		return;
	}

	blocks_[blockCount_].Initialize(position, gravityDirection, motion);
	blocks_[blockCount_].SetImageHandle(imageHandle_);
	++blockCount_;
}
