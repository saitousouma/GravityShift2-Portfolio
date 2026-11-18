#include "EnemyBulletManager.h"

#include "BulletTerrainCollision.h"
#include "GameConfig.h"
#include "GravityBlockManager.h"
#include "Player.h"
#include "StageData.h"
#include "DxLib.h"
#include <cmath>

bool EnemyBulletManager::Load()
{
	Release();

	// 通常弾は描画先画像へ円を描き、カメラ変換できる画像として用意する。
	bulletImageHandle_ = MakeScreen(24, 24, TRUE);
	if (bulletImageHandle_ == -1)
	{
		return false;
	}

	SetDrawScreen(bulletImageHandle_);
	ClearDrawScreen();
	DrawCircle(12, 12, 11, GetColor(255, 230, 80), FALSE);
	DrawCircle(12, 12, 8, GetColor(255, 80, 80), TRUE);
	SetDrawScreen(DX_SCREEN_BACK);

	mortarBulletImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-mortar-bullet.png");

	return true;
}

void EnemyBulletManager::Release()
{
	Clear();

	if (bulletImageHandle_ != -1)
	{
		DeleteGraph(bulletImageHandle_);
		bulletImageHandle_ = -1;
	}
	if (mortarBulletImageHandle_ != -1)
	{
		DeleteGraph(mortarBulletImageHandle_);
		mortarBulletImageHandle_ = -1;
	}
}

void EnemyBulletManager::Shoot(
	const Vector2& startPosition,
	const Vector2& targetPosition)
{
	// 発射位置から目標への方向を正規化し、一定速度の弾にする。
	Vector2 direction{
		targetPosition.x - startPosition.x,
		targetPosition.y - startPosition.y
	};

	const float length =
		std::sqrt(direction.x * direction.x + direction.y * direction.y);

	if (length > 0.0f)
	{
		direction.x /= length;
		direction.y /= length;
	}
	else
	{
		direction.x = 1.0f;
		direction.y = 0.0f;
	}

	SpawnBullet(startPosition,
		{ direction.x * BulletSpeed, direction.y * BulletSpeed },
		EnemyBulletType::Standard);
}

void EnemyBulletManager::ShootMortar(
	const Vector2& startPosition, bool shootDown)
{
	SpawnBullet(startPosition,
		{ 0.0f, shootDown ? MortarVerticalSpeed : -MortarVerticalSpeed },
		EnemyBulletType::MortarVertical);
}

void EnemyBulletManager::ShootVerticalLoop(
	const Vector2& startPosition, bool shootDown)
{
	SpawnBullet(startPosition,
		{ 0.0f, shootDown ? BulletSpeed : -BulletSpeed },
		EnemyBulletType::VerticalLoop);
}

bool EnemyBulletManager::SpawnBullet(
	const Vector2& centerPosition,
	const Vector2& velocity,
	EnemyBulletType type)
{
	for (EnemyBullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			bullet.Initialize(centerPosition, velocity, type);
			return true;
		}
	}
	return false;
}

void EnemyBulletManager::SpawnMortarSplit(const Vector2& centerPosition)
{
	SpawnBullet(centerPosition,
		{ -MortarHorizontalSpeed, 0.0f }, EnemyBulletType::MortarHorizontal);
	SpawnBullet(centerPosition,
		{ MortarHorizontalSpeed, 0.0f }, EnemyBulletType::MortarHorizontal);
}

void EnemyBulletManager::Update(
	const StageData& currentStage,
	const GravityBlockManager& gravityBlockManager)
{
	// 更新中に配列へ弾を追加しないよう、分裂予定の位置を一時保存する。
	std::array<Vector2, MaxEnemyBullets> mortarSplitPositions{};
	int mortarSplitCount = 0;
	for (EnemyBullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		Vector2 previousPosition = bullet.GetPosition();
		bullet.Update();

		if (!bullet.IsActive())
		{
			continue;
		}
		if (bullet.GetType() == EnemyBulletType::VerticalLoop &&
			TryLoopVerticalBullet(bullet, currentStage))
		{
			// ワープ前後を連続移動として判定すると、間の地形まで拾うため、
			// 出口位置をこのフレームの衝突判定開始位置にする。
			previousPosition = bullet.GetPosition();
		}

		const bool isBlockedByStage =
			IsCollidingWithStage(bullet, previousPosition, currentStage);
		const bool isBlockedByGravityBlock =
			gravityBlockManager.CheckEnemyBulletCollision(bullet);

		if (isBlockedByStage || isBlockedByGravityBlock)
		{
			if (isBlockedByStage &&
				bullet.GetType() == EnemyBulletType::MortarVertical &&
				mortarSplitCount < MaxEnemyBullets)
			{
				mortarSplitPositions[mortarSplitCount++] = {
					bullet.GetPosition().x + bullet.GetWidth() * 0.5f,
					previousPosition.y + bullet.GetHeight() * 0.5f
				};
			}
			bullet.Deactivate();
		}
	}

	// 全弾の更新後に分裂弾を生成し、同じフレームでの再更新を防ぐ。
	for (int i = 0; i < mortarSplitCount; ++i)
	{
		SpawnMortarSplit(mortarSplitPositions[i]);
	}
}

bool EnemyBulletManager::TryLoopVerticalBullet(
	EnemyBullet& bullet,
	const StageData& currentStage) const
{
	// まず同じX範囲にある対の穴を探し、見つかれば対応出口へ移す。
	const bool isMovingDown = bullet.GetVelocity().y > 0.0f;
	const float bulletLeft = bullet.GetPosition().x;
	const float bulletRight = bulletLeft + bullet.GetWidth();
	const float bulletTop = bullet.GetPosition().y;
	const float bulletBottom = bulletTop + bullet.GetHeight();
	float exitY = 0.0f;

	if (currentStage.TryGetPairedFallHoleExitY(
		isMovingDown,
		bulletLeft,
		bulletRight,
		bulletTop,
		bulletBottom,
		bullet.GetHeight(),
		exitY))
	{
		bullet.SetPositionY(exitY);
		return true;
	}

	// 対の穴がない旧形式では、天井と床の境界を直接ループさせる。
	bool opensCeiling = false;
	bool opensFloor = false;
	const float bulletCenterX =
		bulletLeft + bullet.GetWidth() * 0.5f;
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& hole = currentStage.fallHoles[i];
		if (bulletCenterX < static_cast<float>(hole.left) ||
			bulletCenterX >= static_cast<float>(hole.right))
		{
			continue;
		}
		opensCeiling = opensCeiling || hole.OpensCeiling();
		opensFloor = opensFloor ||
			hole.OpensFloor(currentStage.GetPixelHeight());
	}

	if (isMovingDown && opensFloor &&
		bulletTop >= static_cast<float>(
			currentStage.GetPixelHeight() + StageData::FallLoopTravelMargin))
	{
		bullet.SetPositionY(-bullet.GetHeight());
		return true;
	}
	if (!isMovingDown && opensCeiling &&
		bulletBottom <= -static_cast<float>(StageData::FallLoopTravelMargin))
	{
		bullet.SetPositionY(static_cast<float>(currentStage.GetPixelHeight()));
		return true;
	}

	return false;
}

bool EnemyBulletManager::IsCollidingWithStage(
	const EnemyBullet& bullet,
	const Vector2& previousPosition,
	const StageData& currentStage) const
{
	// タイルとの衝突に加え、穴がないステージ上下端も壁として扱う。
	const float bulletCenterX =
		bullet.GetPosition().x + bullet.GetWidth() * 0.5f;
	const float bulletTop =
		bullet.GetPosition().y;
	const float bulletBottom =
		bulletTop + bullet.GetHeight();

	if (BulletTerrainCollision::IsBlocked(currentStage,
		previousPosition, bullet.GetPosition(),
		bullet.GetWidth(), bullet.GetHeight()))
	{
		return true;
	}

	bool passesCeiling = false;
	bool passesFloor = false;
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];
		if (bulletCenterX >= static_cast<float>(fallHole.left) &&
			bulletCenterX < static_cast<float>(fallHole.right))
		{
			passesCeiling = passesCeiling || fallHole.OpensCeiling();
			passesFloor = passesFloor ||
				fallHole.OpensFloor(currentStage.GetPixelHeight());
		}
	}

	return (bulletTop <= 0.0f && !passesCeiling) ||
		(bulletBottom >= static_cast<float>(currentStage.GetPixelHeight()) &&
			!passesFloor);
}

void EnemyBulletManager::Draw() const
{
	for (const EnemyBullet& bullet : bullets_)
	{
		bullet.Draw(bulletImageHandle_, mortarBulletImageHandle_);
	}
}

void EnemyBulletManager::Clear()
{
	for (EnemyBullet& bullet : bullets_)
	{
		bullet.Deactivate();
	}
}

bool EnemyBulletManager::CheckPlayerCollision(
	const Player& player,
	float& damageSourceCenterX)
{
	// プレイヤーと重なる最初の弾を消し、ダメージの発生方向を返す。
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();

	for (EnemyBullet& bullet : bullets_)
	{
		if (!bullet.IsActive())
		{
			continue;
		}

		const float bulletLeft = bullet.GetPosition().x;
		const float bulletTop = bullet.GetPosition().y;
		const float bulletRight = bulletLeft + bullet.GetWidth();
		const float bulletBottom = bulletTop + bullet.GetHeight();

		const bool isColliding =
			playerLeft < bulletRight &&
			playerRight > bulletLeft &&
			playerTop < bulletBottom &&
			playerBottom > bulletTop;

		if (isColliding)
		{
			damageSourceCenterX =
				bullet.GetPosition().x + bullet.GetWidth() * 0.5f;
			bullet.Deactivate();
			return true;
		}
	}

	return false;
}

