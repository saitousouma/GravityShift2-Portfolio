#include "EnemyManager.h"

#include "BulletManager.h"
#include "CameraConfig.h"
#include <algorithm>
#include <cmath>
#include "EnemyBulletManager.h"
#include "Laser.h"
#include "GameConfig.h"
#include "EnemySpawnData.h"
#include "EffectManager.h"
#include "ScoreManager.h"
#include "Player.h"
#include "StageData.h"
#include "GravityBlock.h"
#include "DxLib.h"

namespace
{
	constexpr float EnemyDestroyEffectHalfSize = 48.0f;
	constexpr float EnemyDestroyEffectVerticalOffset = 10.0f;
}

void EnemyManager::Initialize(const StageData& currentStage)
{
	damageSoundRequested_ = false;
	deathSoundRequested_ = false;
	// 配置数が0のステージでは、敵を出現させない。
	// 古いステージ番号別の仮配置には戻さず、ステージファイルの内容を優先する。
	for (Enemy& enemy : enemies_)
	{
		enemy.Deactivate();
	}
	ClearHealthPickups();

	for (int i = 0; i < currentStage.enemySpawnCount && i < MaxEnemies; ++i)
	{
		const EnemySpawnData& spawnData =
			currentStage.enemySpawns[i];
		GravityDirection gravityDirection =
			GravityDirection::Down;

		if (spawnData.surface == EnemySpawnSurface::Ceiling)
		{
			gravityDirection = GravityDirection::Up;
		}

		Enemy& enemy = enemies_[i];

		enemy.Initialize(
			spawnData.position,
			spawnData.hitPoint,
			gravityDirection,
			spawnData.type,
			spawnData.isAffectedByGravityReversal,
			spawnData.facing == EnemySpawnFacing::Right ? 1.0f : -1.0f
		);

		enemy.SetImageHandles(
			normalEnemyImageHandle_,
			heavyEnemyImageHandle_,
			shooterEnemyImageHandle_,
			chargerEnemyImageHandle_,
			mortarEnemyImageHandle_
		);
	}
}

void EnemyManager::Initialize(int stageNumber)
{
	damageSoundRequested_ = false;
	deathSoundRequested_ = false;
	for (Enemy& enemy : enemies_)
	{
		enemy.Deactivate();
	}
	ClearHealthPickups();

	// ステージ番号に応じて敵を生成する。
	std::array<EnemySpawnData, MaxEnemies> spawnData{};
	int enemyCount = 0;

	if (stageNumber == 1)
	{
		// ステージ1: 通常敵と射撃敵を1体ずつ。敵弾を避ける練習にする。
		AddSpawnData(spawnData, enemyCount, 470.0f, 1);
		AddSpawnData(
			spawnData,
			enemyCount,
			1040.0f,
			1,
			EnemySpawnSurface::Floor,
			EnemyType::Shooter
		);
	}
	else if (stageNumber == 2)
	{
		// ステージ2: 穴の前後に敵を置き、重力反転で回避する流れを作る。
		AddSpawnData(spawnData, enemyCount, 360.0f, 1);
		AddSpawnData(spawnData, enemyCount, 700.0f, 1);
		AddSpawnData(
			spawnData,
			enemyCount,
			1000.0f,
			1,
			EnemySpawnSurface::Floor,
			EnemyType::Shooter
		);
	}
	else if (stageNumber == 3)
	{
		// ステージ3: 天井敵を混ぜて、重力反転中も敵を意識させる。
		AddSpawnData(spawnData, enemyCount, 460.0f, 1);
		AddSpawnData(spawnData, enemyCount, 670.0f, 1,
			EnemySpawnSurface::Ceiling);
		AddSpawnData(spawnData, enemyCount, 910.0f, 1);
		AddSpawnData(
			spawnData,
			enemyCount,
			1080.0f,
			1,
			EnemySpawnSurface::Floor,
			EnemyType::Shooter
		);
	}
	else if (stageNumber == 4)
	{
		// ステージ4: 中ボス前の道中敵。射撃敵はブロックで弾を防げる位置にする。
		AddSpawnData(spawnData, enemyCount, 450.0f, 1);
		AddSpawnData(spawnData, enemyCount, 730.0f, 1);
		AddSpawnData(
			spawnData,
			enemyCount,
			930.0f,
			1,
			EnemySpawnSurface::Floor,
			EnemyType::Shooter
		);
	}
	else if (stageNumber == 5)
	{
		// ステージ5: 大ボス前の道中敵。重い敵と射撃敵で圧をかける。
		AddSpawnData(spawnData, enemyCount, 330.0f, 1);
		AddSpawnData(spawnData, enemyCount, 690.0f, 1);
		AddSpawnData(
			spawnData,
			enemyCount,
			900.0f,
			2,
			EnemySpawnSurface::Floor,
			EnemyType::Heavy
		);
		AddSpawnData(
			spawnData,
			enemyCount,
			1080.0f,
			1,
			EnemySpawnSurface::Floor,
			EnemyType::Shooter
		);
	}

	for (int i = 0; i < enemyCount; i++)
	{
		Enemy& enemy = enemies_[i];

		const float enemyHeight =
			Enemy::GetHeightByType(spawnData[i].type);

		if (spawnData[i].surface == EnemySpawnSurface::Floor)
		{
			spawnData[i].position.y =
				static_cast<float>(GameConfig::FloorY)
				- enemyHeight;
		}
		else if (spawnData[i].surface == EnemySpawnSurface::Ceiling)
		{
			spawnData[i].position.y =
				static_cast<float>(GameConfig::CeilingY);
		}

		GravityDirection gravityDirection = GravityDirection::Down;

		if (spawnData[i].surface == EnemySpawnSurface::Ceiling)
		{
			gravityDirection = GravityDirection::Up;
		}

		enemy.Initialize(
			spawnData[i].position,
			spawnData[i].hitPoint,
			gravityDirection,
			spawnData[i].type,
			spawnData[i].isAffectedByGravityReversal,
			spawnData[i].facing == EnemySpawnFacing::Right ? 1.0f : -1.0f
		);

		enemy.SetImageHandles(
			normalEnemyImageHandle_,
			heavyEnemyImageHandle_,
			shooterEnemyImageHandle_,
			chargerEnemyImageHandle_,
			mortarEnemyImageHandle_
		);
	}
}

void EnemyManager::AddSpawnData(
	std::array<EnemySpawnData, MaxEnemies>& spawnData,
	int& enemyCount,
	float x,
	int hitPoint,
	EnemySpawnSurface surface,
	EnemyType type
) const
{
	if (enemyCount >= MaxEnemies)
	{
		return;
	}

	spawnData[enemyCount].position = { x, 0.0f };
	spawnData[enemyCount].hitPoint = hitPoint;
	spawnData[enemyCount].surface = surface;
	spawnData[enemyCount].type = type;

	enemyCount++;
}

bool EnemyManager::Load()
{
	normalEnemyImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-normal.png");

	heavyEnemyImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-heavy.png");

	// 射撃敵用画像。まだ画像が無い場合はオレンジの矩形で表示する。
	shooterEnemyImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-shooter.png");
	chargerEnemyImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-charger.png");
	mortarEnemyImageHandle_ =
		LoadGraph(L"Assets/Images/enemy-mortar.png");
	healthPickupImageHandle_ =
		LoadGraph(L"Assets/Images/Items/health-pickup.png");

	for (Enemy& enemy : enemies_)
	{
		enemy.SetImageHandles(
			normalEnemyImageHandle_,
			heavyEnemyImageHandle_,
			shooterEnemyImageHandle_,
			chargerEnemyImageHandle_,
			mortarEnemyImageHandle_
		);
	}

	// 敵画像は未配置でも動くようにする。
	return true;
}

// すべての敵に同じ重力情報を渡し、更新する。
void EnemyManager::Update(
	const GravitySystem& gravitySystem,
	const StageData& currentStage,
	Player& player,
	EnemyBulletManager& enemyBulletManager,
	int cameraX,
	int cameraY)
{
	for (Enemy& enemy : enemies_)
	{
		// カメラから離れた敵は待機させる。
		// 別の階層にいる敵が、見えない場所で移動や射撃を始めるのを防ぐ。
		if (!IsEnemyInsideActivationArea(enemy, cameraX, cameraY))
		{
			continue;
		}

		enemy.Update(gravitySystem, currentStage, player, enemyBulletManager);
	}

	UpdateHealthPickups(gravitySystem, currentStage, player);
}

bool EnemyManager::IsEnemyInsideActivationArea(
	const Enemy& enemy,
	int cameraX,
	int cameraY) const
{
	if (!enemy.IsActive())
	{
		return false;
	}

	const float activationLeft =
		static_cast<float>(cameraX - EnemyActivationMargin);
	const float activationTop =
		static_cast<float>(cameraY - EnemyActivationMargin);
	const float activationRight =
		static_cast<float>(cameraX + CameraConfig::ViewWidth + EnemyActivationMargin);
	const float activationBottom =
		static_cast<float>(cameraY + CameraConfig::ViewHeight + EnemyActivationMargin);

	const float enemyLeft = enemy.GetPosition().x;
	const float enemyTop = enemy.GetPosition().y;
	const float enemyRight = enemyLeft + enemy.GetWidth();
	const float enemyBottom = enemyTop + enemy.GetHeight();

	return enemyLeft < activationRight &&
		enemyRight > activationLeft &&
		enemyTop < activationBottom &&
		enemyBottom > activationTop;
}

// すべての敵を描画する。
void EnemyManager::Draw() const
{
	for (const Enemy& enemy : enemies_)
	{
		enemy.Draw();
	}
	DrawHealthPickups();
}

// 重力反転時に、すべての敵の縦速度をリセットする。
void EnemyManager::OnGravityReversed()
{
	for (Enemy& enemy : enemies_)
	{
		enemy.OnGravityReversed();
	}
}

// 弾とレーザーの当たり判定を行う。
void EnemyManager::CheckWeaponCollisions(
	BulletManager& bulletManager,
	Laser& laser,
	EffectManager& effectManager,
	ScoreManager& scoreManager
)
{
	bool didLaserDealDamage = false;

	for (Enemy& enemy : enemies_)
	{
		if (!enemy.IsActive())
		{
			continue;
		}

		const bool wasActive = enemy.IsActive();
		const int previousHitPoint = enemy.GetHitPoint();
		const Vector2 effectPosition{
			enemy.GetPosition().x + enemy.GetWidth() * 0.5f - EnemyDestroyEffectHalfSize,
			enemy.GetPosition().y + enemy.GetHeight() * 0.5f -
				EnemyDestroyEffectHalfSize - EnemyDestroyEffectVerticalOffset
		};

		bulletManager.CheckCollision(enemy);

		const bool isLaserHit =
			laser.CheckCollision(enemy);

		if (isLaserHit)
		{
			didLaserDealDamage = true;
		}

		const bool isDefeatedNow =
			wasActive && !enemy.IsActive();
		if (enemy.GetHitPoint() < previousHitPoint)
		{
			if (isDefeatedNow)
			{
				deathSoundRequested_ = true;
			}
			else
			{
				damageSoundRequested_ = true;
			}
		}

		if (isDefeatedNow)
		{
			effectManager.PlayEnemyDestroyEffect(effectPosition);
			TrySpawnHealthPickup(enemy);

			// スコアを表示する場所を決定。
			const Vector2 scorePopupPosition
			{
				enemy.GetPosition().x + enemy.GetWidth() * 0.5f,
				enemy.GetPosition().y
			};

			// 通常敵のスコア。
			int score = 100;
			int scoreColor = GetColor(255, 230, 80);

			// Heavy敵とShooter敵は、通常敵より高いスコアにする。
			if (enemy.GetType() == EnemyType::Heavy)
			{
				score = 300;
				scoreColor = GetColor(255, 150, 60);
			}
			else if (enemy.GetType() == EnemyType::Shooter ||
				enemy.GetType() == EnemyType::UpShooter)
			{
				score = 200;
				scoreColor = GetColor(255, 190, 80);
			}
			else if (enemy.GetType() == EnemyType::Mortar)
			{
				score = 250;
				scoreColor = GetColor(100, 230, 255);
			}

			scoreManager.AddScore(score, scorePopupPosition, 20, scoreColor);
		}
	}

	if (didLaserDealDamage)
	{
		laser.OnDamageDealt();
	}
}

// 重力ブロックが落下中に敵へ当たったら、その敵を押しつぶして倒す。
void EnemyManager::CheckGravityBlockCollisions(
	const GravityBlock& gravityBlock,
	EffectManager& effectManager,
	ScoreManager& scoreManager)
{
	if (!gravityBlock.IsFalling())
	{
		return;
	}

	const float blockLeft = gravityBlock.GetPosition().x;
	const float blockTop = gravityBlock.GetPosition().y;
	const float blockRight = blockLeft + gravityBlock.GetWidth();
	const float blockBottom = blockTop + gravityBlock.GetHeight();

	for (Enemy& enemy : enemies_)
	{
		if (!enemy.IsActive())
		{
			continue;
		}

		const float enemyLeft = enemy.GetPosition().x;
		const float enemyTop = enemy.GetPosition().y;
		const float enemyRight = enemyLeft + enemy.GetWidth();
		const float enemyBottom = enemyTop + enemy.GetHeight();

		const bool isColliding =
			blockLeft < enemyRight &&
			blockRight > enemyLeft &&
			blockTop < enemyBottom &&
			blockBottom > enemyTop;

		if (!isColliding)
		{
			continue;
		}

		const Vector2 effectPosition
		{
			enemy.GetPosition().x + enemy.GetWidth() * 0.5f - EnemyDestroyEffectHalfSize,
			enemy.GetPosition().y + enemy.GetHeight() * 0.5f -
				EnemyDestroyEffectHalfSize - EnemyDestroyEffectVerticalOffset
		};

		effectManager.PlayEnemyDestroyEffect(effectPosition);
		TrySpawnHealthPickup(enemy);

		const Vector2 scorePopupPosition
		{
			enemy.GetPosition().x + enemy.GetWidth() * 0.5f,
			enemy.GetPosition().y
		};

		int score = 100;
		int scoreColor = GetColor(255, 230, 80);

		if (enemy.GetType() == EnemyType::Heavy)
		{
			score = 300;
			scoreColor = GetColor(255, 150, 60);
		}
		else if (enemy.GetType() == EnemyType::Shooter ||
			enemy.GetType() == EnemyType::UpShooter)
		{
			score = 200;
			scoreColor = GetColor(255, 190, 80);
		}
		else if (enemy.GetType() == EnemyType::Mortar)
		{
			score = 250;
			scoreColor = GetColor(100, 230, 255);
		}

		enemy.Deactivate();
		deathSoundRequested_ = true;
		scoreManager.AddScore(score, scorePopupPosition, 20, scoreColor);
	}
}

bool EnemyManager::ConsumeDamageSoundRequest()
{
	const bool requested = damageSoundRequested_;
	damageSoundRequested_ = false;
	return requested;
}

bool EnemyManager::ConsumeDeathSoundRequest()
{
	const bool requested = deathSoundRequested_;
	deathSoundRequested_ = false;
	return requested;
}

// プレイヤーが有効な敵に触れているか調べる。
bool EnemyManager::CheckPlayerCollision(
	const Player& player,
	float& damageSourceCenterX) const
{
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();

	for (const Enemy& enemy : enemies_)
	{
		if (!enemy.IsActive())
		{
			continue;
		}

		const float enemyLeft = enemy.GetPosition().x;
		const float enemyTop = enemy.GetPosition().y;
		const float enemyRight = enemyLeft + enemy.GetWidth();
		const float enemyBottom = enemyTop + enemy.GetHeight();

		const bool isColliding =
			playerLeft < enemyRight &&
			playerRight > enemyLeft &&
			playerTop < enemyBottom &&
			playerBottom > enemyTop;

		if (isColliding)
		{
			damageSourceCenterX =
				enemy.GetPosition().x + enemy.GetWidth() * 0.5f;
			return true;
		}
	}

	return false;
}

// すべての敵が非アクティブならtrueを返す。
bool EnemyManager::IsAllDefeated() const
{
	for (const Enemy& enemy : enemies_)
	{
		if (enemy.IsActive())
		{
			return false;
		}
	}

	return true;
}

void EnemyManager::ClearHealthPickups()
{
	for (HealthPickup& pickup : healthPickups_)
	{
		pickup = {};
	}
}

void EnemyManager::TrySpawnHealthPickup(const Enemy& enemy)
{
	if (GetRand(99) >= HealthPickupDropChancePercent)
	{
		return;
	}

	for (HealthPickup& pickup : healthPickups_)
	{
		if (pickup.isActive)
		{
			continue;
		}

		pickup.position.x = enemy.GetPosition().x + enemy.GetWidth() * 0.5f -
			HealthPickupWidth * 0.5f;
		pickup.position.y = enemy.GetPosition().y + enemy.GetHeight() * 0.5f -
			HealthPickupHeight * 0.5f;
		pickup.velocityY = 0.0f;
		pickup.isActive = true;
		return;
	}
}

void EnemyManager::UpdateHealthPickups(
	const GravitySystem& gravitySystem,
	const StageData& currentStage,
	Player& player)
{
	const float gravitySign = gravitySystem.GetDirection() == GravityDirection::Down
		? 1.0f : -1.0f;
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();

	for (HealthPickup& pickup : healthPickups_)
	{
		if (!pickup.isActive)
		{
			continue;
		}

		pickup.velocityY += HealthPickupGravity * gravitySign;
		pickup.velocityY = (std::max)(-HealthPickupMaxFallSpeed,
			(std::min)(HealthPickupMaxFallSpeed, pickup.velocityY));
		const float nextY = pickup.position.y + pickup.velocityY;
		int collisionRow = -1;
		if (IsHealthPickupTouchingSolid(
			pickup, nextY, currentStage, collisionRow))
		{
			pickup.position.y = gravitySign > 0.0f
				? static_cast<float>(collisionRow * StageData::TileSize) -
					HealthPickupHeight
				: static_cast<float>((collisionRow + 1) * StageData::TileSize);
			pickup.velocityY = 0.0f;
		}
		else
		{
			pickup.position.y = nextY;
		}

		if (pickup.position.y > currentStage.GetPixelHeight() + HealthPickupHeight ||
			pickup.position.y + HealthPickupHeight < -HealthPickupHeight)
		{
			pickup.isActive = false;
			continue;
		}

		const float pickupRight = pickup.position.x + HealthPickupWidth;
		const float pickupBottom = pickup.position.y + HealthPickupHeight;
		const bool isTouchingPlayer =
			pickup.position.x < playerRight &&
			pickupRight > playerLeft &&
			pickup.position.y < playerBottom &&
			pickupBottom > playerTop;
		if (isTouchingPlayer)
		{
			player.Heal();
			pickup.isActive = false;
		}
	}
}

bool EnemyManager::IsHealthPickupTouchingSolid(
	const HealthPickup& pickup,
	float nextY,
	const StageData& currentStage,
	int& collisionRow) const
{
	const bool isMovingDown = pickup.velocityY >= 0.0f;
	const float leadingY = isMovingDown
		? nextY + HealthPickupHeight
		: nextY;
	const int row = static_cast<int>(std::floor(
		leadingY / static_cast<float>(StageData::TileSize)));
	if (row < 0 || row >= currentStage.tileRowCount)
	{
		return false;
	}

	const int firstColumn = (std::max)(0, static_cast<int>(std::floor(
		(pickup.position.x + 5.0f) / StageData::TileSize)));
	const int lastColumn = (std::min)(StageData::TileColumns - 1,
		static_cast<int>(std::floor(
			(pickup.position.x + HealthPickupWidth - 5.0f) /
			StageData::TileSize)));
	for (int column = firstColumn; column <= lastColumn; ++column)
	{
		if (StageData::IsSolidTile(currentStage.tiles[row][column]))
		{
			collisionRow = row;
			return true;
		}
	}
	return false;
}

void EnemyManager::DrawHealthPickups() const
{
	for (const HealthPickup& pickup : healthPickups_)
	{
		if (!pickup.isActive)
		{
			continue;
		}

		const int left = static_cast<int>(pickup.position.x);
		const int top = static_cast<int>(pickup.position.y);
		const int right = static_cast<int>(pickup.position.x + HealthPickupWidth);
		const int bottom = static_cast<int>(pickup.position.y + HealthPickupHeight);
		if (healthPickupImageHandle_ != -1)
		{
			DrawExtendGraph(left, top, right, bottom,
				healthPickupImageHandle_, TRUE);
		}
		else
		{
			DrawCircle((left + right) / 2, (top + bottom) / 2,
				16, GetColor(80, 255, 120), TRUE);
		}
	}
}
