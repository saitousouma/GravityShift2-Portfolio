#include "BulletManager.h"
#include "Enemy.h"
#include "Boss.h"
#include "BulletTerrainCollision.h"
#include "GravityBlockManager.h"
#include "DxLib.h"

// 未使用の弾を1つ探して発射する。
// すべて使用中の場合は何もせず終了する。
void BulletManager::Shoot(const Vector2& position,
	const Vector2& direction)
{
	for (Bullet& bullet : bullets_)
	{
		if (bullet.IsActive())
		{
			continue;
		}

		bullet.Initialize(position, direction);
		return;
	}
}

// 有効な弾だけを更新する。
// Bullet側にも有効判定を残し、管理側と弾側の両方で安全にしている。
void BulletManager::Update(const StageData& currentStage,
	const GravityBlockManager& gravityBlockManager)
{
	for (Bullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		const Vector2 previousPosition = bullet.GetPosition();
		bullet.Update();
		if (bullet.IsActive() &&
			(BulletTerrainCollision::IsBlocked(currentStage,
				previousPosition, bullet.GetPosition(),
				bullet.GetWidth(), bullet.GetHeight()) ||
			gravityBlockManager.CheckPlayerBulletCollision(
				bullet, previousPosition)))
		{
			bullet.Deactivate();
			continue;
		}

		if (!bullet.IsActive())
		{
			continue;
		}

		const Vector2& direction = bullet.GetDirection();
		const Vector2& position = bullet.GetPosition();
		bool passedPairedHole = false;
		if (direction.y != 0.0f)
		{
			float exitY = 0.0f;
			passedPairedHole = currentStage.TryGetPairedFallHoleExitY(
				direction.y > 0.0f,
				position.x,
				position.x + bullet.GetWidth(),
				position.y,
				position.y + bullet.GetHeight(),
				bullet.GetHeight(),
				exitY);
			if (passedPairedHole)
			{
				bullet.SetPositionY(exitY);
			}
		}

		if (!passedPairedHole &&
			(position.y + bullet.GetHeight() < 0.0f ||
				position.y > static_cast<float>(currentStage.GetPixelHeight())))
		{
			bullet.Deactivate();
		}
	}
}

// ステージ開始時に残っている弾をすべて未使用状態に戻す。
void BulletManager::Clear()
{
	for (Bullet& bullet : bullets_)
	{
		bullet.Deactivate();
	}
}
void BulletManager::CheckCollision(Enemy& enemy)
{
	if (!enemy.IsActive())
	{
		return;
	}

	const Vector2& enemyPosition = enemy.GetPosition();


	const float enemyLeft = enemyPosition.x;
	const float enemyTop = enemyPosition.y;
	const float enemyRight = enemyLeft + enemy.GetWidth();
	const float enemyBottom = enemyTop + enemy.GetHeight();

	for (Bullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		const Vector2& bulletPosition = bullet.GetPosition();

		const float bulletLeft = bulletPosition.x;
		const float bulletTop = bulletPosition.y;
		const float bulletRight = bulletLeft + bullet.GetWidth();
		const float bulletBottom = bulletTop + bullet.GetHeight();

		// 2つの矩形が上下左右すべてで重なっているか調べる。
		const bool isHit =
			bulletLeft < enemyRight &&
			bulletRight > enemyLeft &&
			bulletTop < enemyBottom &&
			bulletBottom > enemyTop;

		if (!isHit)
		{
			continue;
		}

		bullet.Deactivate();
		enemy.TakeDamage();
		return;
	}
}

// 発射中の弾とボスの当たり判定を調べる。
void BulletManager::CheckCollision(Boss& boss)
{
	if (!boss.IsActive())
	{
		return;
	}

	for (Bullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		const Vector2& bulletPosition = bullet.GetPosition();
		const Vector2& bossPosition = boss.GetPosition();

		const float bulletLeft = bulletPosition.x;
		const float bulletTop = bulletPosition.y;
		const float bulletRight = bulletLeft + bullet.GetWidth();
		const float bulletBottom = bulletTop + bullet.GetHeight();

		const float bossLeft = bossPosition.x;
		const float bossTop = bossPosition.y;
		const float bossRight = bossLeft + boss.GetWidth();
		const float bossBottom = bossTop + boss.GetHeight();

		const bool isHit =
			bulletLeft < bossRight &&
			bulletRight > bossLeft &&
			bulletTop < bossBottom &&
			bulletBottom > bossTop;

		if (isHit)
		{
			bullet.Deactivate();
			boss.TakeDamage();
			return;
		}
	}
}

// 有効な弾だけを描画する。
// Drawは状態を変更しないため、const参照で弾を読む。
void BulletManager::Draw() const
{
	for (const Bullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		bullet.Draw();
	}
}

// 弾用の画像を読み込み、全ての弾へ共有ハンドルを渡す。
bool BulletManager::Load()
{
	imageHandle_ = LoadGraph(L"Assets/Images/weapon-effects.png");

	if (imageHandle_ == -1)
	{
		return false;
	}

	for (Bullet& bullet : bullets_)
	{
		bullet.SetImageHandle(imageHandle_);
	}

	return true;
}

int BulletManager::GetImageHandle() const
{
	return imageHandle_;
}

// 読み込んだ弾画像を開放する。
void BulletManager::Release()
{
	if (imageHandle_ == -1)
	{
		return;
	}

	DeleteGraph(imageHandle_);
	imageHandle_ = -1;
}
