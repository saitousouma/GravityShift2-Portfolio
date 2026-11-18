#include "Boss.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "GameConfig.h"
#include "Player.h"
#include "StageData.h"

namespace
{
	constexpr float ProjectileSize = 24.0f;
	constexpr float ExplosionSize = 96.0f;
	constexpr float ProjectileGravity = 0.32f;
	constexpr float MaximumProjectileFallSpeed = 8.0f;
}

bool Boss::Load()
{
	// 中ボス、大ボス、攻撃物の画像をまとめて読み込む。
	SetTransColor(255, 255, 255);
	idleImageHandle_ = LoadGraph(L"Assets/Images/Boss/midboss-idle.png");
	walkImageHandle_ = LoadGraph(L"Assets/Images/Boss/midboss-walk.png");
	mineImageHandle_ = LoadGraph(L"Assets/Images/Boss/midboss-mine.png");
	grenadeImageHandle_ = LoadGraph(L"Assets/Images/Boss/midboss-grenade.png");
	tankMoveImageHandle_ = LoadGraph(L"Assets/Images/Boss/tank-move.png");
	tankFireImageHandle_ = LoadGraph(L"Assets/Images/Boss/tank-fire.png");
	attackWarningImageHandle_ =
		LoadGraph(L"Assets/Images/Effects/enemy-attack-warning.png");
	if (idleImageHandle_ == -1)
	{
		idleImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/midboss-idle.png");
	}
	if (walkImageHandle_ == -1)
	{
		walkImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/midboss-walk.png");
	}
	if (mineImageHandle_ == -1)
	{
		mineImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/midboss-mine.png");
	}
	if (grenadeImageHandle_ == -1)
	{
		grenadeImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/midboss-grenade.png");
	}
	if (tankMoveImageHandle_ == -1)
	{
		tankMoveImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/tank-move.png");
	}
	if (tankFireImageHandle_ == -1)
	{
		tankFireImageHandle_ = LoadGraph(L"../../Assets/Images/Boss/tank-fire.png");
	}
	if (attackWarningImageHandle_ == -1)
	{
		attackWarningImageHandle_ =
			LoadGraph(L"../../Assets/Images/Effects/enemy-attack-warning.png");
	}
	SetTransColor(0, 0, 0);
	return true;
}

void Boss::Release()
{
	// 読み込まれている画像だけを解放し、ハンドルを無効値へ戻す。
	if (idleImageHandle_ != -1)
	{
		DeleteGraph(idleImageHandle_);
		idleImageHandle_ = -1;
	}
	if (walkImageHandle_ != -1)
	{
		DeleteGraph(walkImageHandle_);
		walkImageHandle_ = -1;
	}
	if (mineImageHandle_ != -1)
	{
		DeleteGraph(mineImageHandle_);
		mineImageHandle_ = -1;
	}
	if (grenadeImageHandle_ != -1)
	{
		DeleteGraph(grenadeImageHandle_);
		grenadeImageHandle_ = -1;
	}
	if (attackWarningImageHandle_ != -1)
	{
		DeleteGraph(attackWarningImageHandle_);
		attackWarningImageHandle_ = -1;
	}
	if (tankMoveImageHandle_ != -1)
	{
		DeleteGraph(tankMoveImageHandle_);
		tankMoveImageHandle_ = -1;
	}
	if (tankFireImageHandle_ != -1)
	{
		DeleteGraph(tankFireImageHandle_);
		tankFireImageHandle_ = -1;
	}
}

void Boss::Initialize(BossType type)
{
	// エディター設定を使わない場合の標準状態を作る。
	type_ = type;
	velocity_ = {};
	hitFlashFrames_ = 0;
	animationTimer_ = 0;
	attackCooldownFrames_ = type == BossType::Boss
		? 90 : AttackCooldownDuration;
	attackWindupFrames_ = 0;
	completedAttackCount_ = 0;
	pendingGrenadeAttack_ = false;
	pendingMineDashAttack_ = false;
	pendingPincerAttack_ = false;
	pendingRicochetAttack_ = false;
	grenadeSoundRequested_ = false;
	attackWarningSoundRequested_ = false;
	isMineDashActive_ = false;
	mineDashDropTimer_ = 0;
	tankShotFired_ = false;
	tankAttackPattern_ = TankAttackPattern::Cannon;
	tankShotsFired_ = 0;
	tankGravityMismatchFrames_ = 0;
	tankGravityFlipCooldownFrames_ = 0;
	tankAttackTarget_ = {};
	ClearProjectiles();
	isInitialized_ = true;
	hasEncounterStarted_ = true;
	isActive_ = true;
	isDefeated_ = false;
	width_ = type_ == BossType::MidBoss
		? static_cast<float>(MidBossSpawnData::Size) : 180.0f;
	height_ = type_ == BossType::MidBoss ? width_ : 120.0f;
	maxHitPoint_ = type_ == BossType::MidBoss
		? MidBossSpawnData::DefaultHitPoint : BossSpawnData::DefaultHitPoint;
	hitPoint_ = maxHitPoint_;
	position_.x = 900.0f;
	position_.y = static_cast<float>(GameConfig::FloorY) - height_;
	ownGravityDirection_ = GravityDirection::Down;
	facing_ = EnemySpawnFacing::Left;
}

void Boss::InitializeMidBoss(const MidBossSpawnData& spawnData)
{
	// 中ボスの配置、HP、接地面、向きをステージ設定から反映する。
	type_ = BossType::MidBoss;
	position_ = spawnData.position;
	velocity_ = {};
	width_ = static_cast<float>(MidBossSpawnData::Size);
	height_ = static_cast<float>(MidBossSpawnData::Size);
	maxHitPoint_ = (std::max)(1, spawnData.hitPoint);
	hitPoint_ = maxHitPoint_;
	hitFlashFrames_ = 0;
	animationTimer_ = 0;
	attackCooldownFrames_ = 90;
	attackWindupFrames_ = 0;
	completedAttackCount_ = 0;
	pendingGrenadeAttack_ = false;
	pendingMineDashAttack_ = false;
	pendingPincerAttack_ = false;
	pendingRicochetAttack_ = false;
	grenadeSoundRequested_ = false;
	attackWarningSoundRequested_ = false;
	isMineDashActive_ = false;
	mineDashDropTimer_ = 0;
	tankShotFired_ = false;
	tankAttackPattern_ = TankAttackPattern::Cannon;
	tankShotsFired_ = 0;
	tankGravityMismatchFrames_ = 0;
	tankGravityFlipCooldownFrames_ = 0;
	tankAttackTarget_ = {};
	ClearProjectiles();
	ownGravityDirection_ = spawnData.surface == EnemySpawnSurface::Ceiling
		? GravityDirection::Up : GravityDirection::Down;
	facing_ = spawnData.facing;
	isInitialized_ = true;
	hasEncounterStarted_ = false;
	isActive_ = false;
	isDefeated_ = false;
}

void Boss::InitializeBoss(const BossSpawnData& spawnData)
{
	// 大ボスの配置、HP、向きをステージ設定から反映する。
	type_ = BossType::Boss;
	position_ = spawnData.position;
	velocity_ = {};
	width_ = static_cast<float>(BossSpawnData::Width);
	height_ = static_cast<float>(BossSpawnData::Height);
	maxHitPoint_ = (std::max)(1, spawnData.hitPoint);
	hitPoint_ = maxHitPoint_;
	hitFlashFrames_ = 0;
	animationTimer_ = 0;
	attackCooldownFrames_ = 90;
	attackWindupFrames_ = 0;
	completedAttackCount_ = 0;
	pendingGrenadeAttack_ = false;
	pendingMineDashAttack_ = false;
	pendingPincerAttack_ = false;
	pendingRicochetAttack_ = false;
	grenadeSoundRequested_ = false;
	attackWarningSoundRequested_ = false;
	isMineDashActive_ = false;
	mineDashDropTimer_ = 0;
	tankShotFired_ = false;
	tankAttackPattern_ = TankAttackPattern::Cannon;
	tankShotsFired_ = 0;
	tankGravityMismatchFrames_ = 0;
	tankGravityFlipCooldownFrames_ = 0;
	tankAttackTarget_ = {};
	ClearProjectiles();
	ownGravityDirection_ = GravityDirection::Down;
	facing_ = spawnData.facing;
	isInitialized_ = true;
	hasEncounterStarted_ = false;
	isActive_ = false;
	isDefeated_ = false;
}

void Boss::StartEncounter()
{
	// 配置直後は待機させ、戦闘範囲へ入った時だけ行動を有効にする。
	if (!isInitialized_ || hasEncounterStarted_ || isDefeated_)
	{
		return;
	}
	hasEncounterStarted_ = true;
	isActive_ = true;
	attackCooldownFrames_ = 90;
}

void Boss::Update(const GravitySystem& gravitySystem, const StageData& stageData,
	const Player& player)
{
	// 共通タイマーを進めた後、種類に対応する行動処理へ分岐する。
	if (!isActive_)
	{
		return;
	}
	if (hitFlashFrames_ > 0)
	{
		--hitFlashFrames_;
	}
	++animationTimer_;

	if (type_ == BossType::MidBoss)
	{
		UpdateMidBoss(stageData, player);
		return;
	}
	UpdateTankBoss(gravitySystem, stageData, player);
}

void Boss::UpdateMidBoss(const StageData& stageData, const Player& player)
{
	// プレイヤーの方向を向き、歩行・自重力反転・攻撃を更新する。
	constexpr float walkSpeed = 1.5f;
	constexpr float gravityAcceleration = 0.35f;
	constexpr float maxFallSpeed = 5.0f;
	constexpr float facingDeadZone = 12.0f;
	UpdateProjectiles(stageData);
	if (!isMineDashActive_)
	{
		const float playerCenterX =
			player.GetPosition().x + player.GetWidth() * 0.5f;
		const float bossCenterX = position_.x + width_ * 0.5f;
		if (playerCenterX < bossCenterX - facingDeadZone)
		{
			facing_ = EnemySpawnFacing::Left;
		}
		else if (playerCenterX > bossCenterX + facingDeadZone)
		{
			facing_ = EnemySpawnFacing::Right;
		}
	}
	UpdateAttack();
	const float moveSpeed = isMineDashActive_ ? MineDashSpeed : walkSpeed;
	velocity_.x = attackWindupFrames_ > 0
		? 0.0f
		: (facing_ == EnemySpawnFacing::Right ? moveSpeed : -moveSpeed);

	const float nextX = position_.x + velocity_.x;
	const int edgeColumn = static_cast<int>(
		(velocity_.x > 0.0f ? nextX + width_ - 1.0f : nextX) /
		StageData::TileSize);
	const int firstRow = static_cast<int>(position_.y / StageData::TileSize);
	const int lastRow = static_cast<int>(
		(position_.y + height_ - 1.0f) / StageData::TileSize);
	bool hitWall = nextX < 0.0f || nextX + width_ > StageData::StagePixelWidth;
	for (int row = firstRow; row <= lastRow && !hitWall; ++row)
	{
		hitWall = IsSolidAt(stageData, edgeColumn, row);
	}
	const int supportRow = ownGravityDirection_ == GravityDirection::Down
		? static_cast<int>((position_.y + height_ + 1.0f) /
			StageData::TileSize)
		: static_cast<int>(std::floor((position_.y - 1.0f) /
			StageData::TileSize));
	const int centerColumn = static_cast<int>(
		(position_.x + width_ * 0.5f) / StageData::TileSize);
	if (!hitWall && IsSolidAt(stageData, centerColumn, supportRow) &&
		!IsSolidAt(stageData, edgeColumn, supportRow))
	{
		hitWall = true;
	}
	if (hitWall)
	{
		facing_ = facing_ == EnemySpawnFacing::Left
			? EnemySpawnFacing::Right : EnemySpawnFacing::Left;
		velocity_.x = 0.0f;
		if (isMineDashActive_)
		{
			isMineDashActive_ = false;
			mineDashDropTimer_ = 0;
			++completedAttackCount_;
			attackCooldownFrames_ = AttackCooldownDuration;
			if (completedAttackCount_ % 2 == 0)
			{
				ReverseOwnGravity();
			}
		}
	}
	else
	{
		position_.x = nextX;
	}

	const float gravitySign = ownGravityDirection_ == GravityDirection::Down
		? 1.0f : -1.0f;
	velocity_.y += gravityAcceleration * gravitySign;
	velocity_.y = (std::max)(-maxFallSpeed,
		(std::min)(maxFallSpeed, velocity_.y));
	float nextY = position_.y + velocity_.y;
	const int firstColumn = static_cast<int>(position_.x / StageData::TileSize);
	const int lastColumn = static_cast<int>(
		(position_.x + width_ - 1.0f) / StageData::TileSize);

	if (ownGravityDirection_ == GravityDirection::Down)
	{
		const int row = static_cast<int>((nextY + height_ - 1.0f) /
			StageData::TileSize);
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (IsSolidAt(stageData, column, row))
			{
				nextY = static_cast<float>(row * StageData::TileSize) - height_;
				velocity_.y = 0.0f;
				break;
			}
		}
	}
	else
	{
		const int row = static_cast<int>(std::floor(nextY / StageData::TileSize));
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (IsSolidAt(stageData, column, row))
			{
				nextY = static_cast<float>((row + 1) * StageData::TileSize);
				velocity_.y = 0.0f;
				break;
			}
		}
	}
	position_.y = nextY;
}

void Boss::UpdateTankBoss(const GravitySystem& gravitySystem,
	const StageData& stageData, const Player& player)
{
	// プレイヤーの位置と接地面に応じて追跡、重力反転、攻撃を行う。
	constexpr float walkSpeed = 1.2f;
	constexpr float gravityAcceleration = 0.35f;
	constexpr float maxFallSpeed = 6.0f;
	constexpr float facingDeadZone = 12.0f;
	UpdateProjectiles(stageData);
	if (tankGravityFlipCooldownFrames_ > 0)
	{
		--tankGravityFlipCooldownFrames_;
	}
	bool playerOnOppositeSurface =
		gravitySystem.GetDirection() != ownGravityDirection_;
	if (playerOnOppositeSurface)
	{
		++tankGravityMismatchFrames_;
		if (tankGravityMismatchFrames_ >= TankGravityFlipDelayDuration &&
			tankGravityFlipCooldownFrames_ <= 0 && attackWindupFrames_ <= 0)
		{
			ReverseOwnGravity();
			tankGravityMismatchFrames_ = 0;
			tankGravityFlipCooldownFrames_ = TankGravityFlipCooldownDuration;
			playerOnOppositeSurface = false;
		}
	}
	else
	{
		tankGravityMismatchFrames_ = 0;
	}
	if (attackWindupFrames_ <= 0)
	{
		const float playerCenterX =
			player.GetPosition().x + player.GetWidth() * 0.5f;
		const float bossCenterX = position_.x + width_ * 0.5f;
		if (playerCenterX < bossCenterX - facingDeadZone)
		{
			facing_ = EnemySpawnFacing::Left;
		}
		else if (playerCenterX > bossCenterX + facingDeadZone)
		{
			facing_ = EnemySpawnFacing::Right;
		}
	}
	UpdateTankAttack(player, playerOnOppositeSurface);
	velocity_.x = attackWindupFrames_ > 0
		? 0.0f
		: (facing_ == EnemySpawnFacing::Right ? walkSpeed : -walkSpeed);

	const float nextX = position_.x + velocity_.x;
	const int edgeColumn = static_cast<int>(std::floor(
		(velocity_.x > 0.0f ? nextX + width_ - 1.0f : nextX) /
		StageData::TileSize));
	const int firstRow = static_cast<int>(std::floor(
		position_.y / StageData::TileSize));
	const int lastRow = static_cast<int>(std::floor(
		(position_.y + height_ - 1.0f) / StageData::TileSize));
	bool hitWall = nextX < 0.0f ||
		nextX + width_ > StageData::StagePixelWidth;
	for (int row = firstRow; row <= lastRow && !hitWall; ++row)
	{
		hitWall = IsSolidAt(stageData, edgeColumn, row);
	}

	const int supportRow = ownGravityDirection_ == GravityDirection::Down
		? static_cast<int>(std::floor(
			(position_.y + height_ + 1.0f) / StageData::TileSize))
		: static_cast<int>(std::floor(
			(position_.y - 1.0f) / StageData::TileSize));
	const int nextCenterColumn = static_cast<int>(std::floor(
		(nextX + width_ * 0.5f) / StageData::TileSize));
	if (!hitWall && !IsSolidAt(stageData, nextCenterColumn, supportRow))
	{
		hitWall = true;
	}

	if (hitWall)
	{
		velocity_.x = 0.0f;
	}
	else
	{
		position_.x = nextX;
	}

	const float gravitySign = ownGravityDirection_ == GravityDirection::Down
		? 1.0f : -1.0f;
	velocity_.y += gravityAcceleration * gravitySign;
	velocity_.y = (std::max)(-maxFallSpeed,
		(std::min)(maxFallSpeed, velocity_.y));
	float nextY = position_.y + velocity_.y;
	const int firstColumn = static_cast<int>(std::floor(
		position_.x / StageData::TileSize));
	const int lastColumn = static_cast<int>(std::floor(
		(position_.x + width_ - 1.0f) / StageData::TileSize));
	if (ownGravityDirection_ == GravityDirection::Down)
	{
		const int collisionRow = static_cast<int>(std::floor(
			(nextY + height_ - 1.0f) / StageData::TileSize));
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (IsSolidAt(stageData, column, collisionRow))
			{
				nextY = static_cast<float>(collisionRow * StageData::TileSize) -
					height_;
				velocity_.y = 0.0f;
				break;
			}
		}
	}
	else
	{
		const int collisionRow = static_cast<int>(std::floor(
			nextY / StageData::TileSize));
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (IsSolidAt(stageData, column, collisionRow))
			{
				nextY = static_cast<float>(
					(collisionRow + 1) * StageData::TileSize);
				velocity_.y = 0.0f;
				break;
			}
		}
	}
	position_.y = nextY;
}

void Boss::UpdateTankAttack(const Player& player,
	bool playerOnOppositeSurface)
{
	// 攻撃予告後、選択中のパターンに対応する砲撃を発生させる。
	if (attackWindupFrames_ > 0)
	{
		const int elapsedFrames = TankFireDuration - attackWindupFrames_;
		const int firstFireFrame = TankFireFrame * TankAnimationFrameDuration;
		switch (tankAttackPattern_)
		{
		case TankAttackPattern::Burst:
		{
			const int shotCount = hitPoint_ * 2 <= maxHitPoint_ ? 4 : 3;
			while (tankShotsFired_ < shotCount &&
				elapsedFrames >= firstFireFrame +
					tankShotsFired_ * TankBurstIntervalDuration)
			{
				SpawnTankShell();
				++tankShotsFired_;
				tankShotFired_ = true;
			}
			break;
		}
		case TankAttackPattern::GrenadeVolley:
			if (!tankShotFired_ && elapsedFrames >= firstFireFrame)
			{
				SpawnTankGrenadeVolley();
				tankShotFired_ = true;
			}
			break;
		case TankAttackPattern::MineDrop:
			if (!tankShotFired_ && elapsedFrames >= firstFireFrame)
			{
				SpawnTankMines();
				tankShotFired_ = true;
			}
			break;
		case TankAttackPattern::AntiSurfaceShot:
			if (!tankShotFired_ && elapsedFrames >= firstFireFrame)
			{
				SpawnTankAimedShell();
				tankShotFired_ = true;
			}
			break;
		default:
			if (!tankShotFired_ && elapsedFrames >= firstFireFrame)
			{
				SpawnTankShell();
				tankShotFired_ = true;
			}
			break;
		}

		--attackWindupFrames_;
		if (attackWindupFrames_ == 0)
		{
			++completedAttackCount_;
			attackCooldownFrames_ = hitPoint_ * 2 <= maxHitPoint_
				? TankEnragedAttackCooldownDuration
				: TankAttackCooldownDuration;
		}
		return;
	}

	if (attackCooldownFrames_ > 0)
	{
		--attackCooldownFrames_;
		return;
	}

	tankAttackPattern_ = playerOnOppositeSurface
		? TankAttackPattern::AntiSurfaceShot
		: static_cast<TankAttackPattern>(completedAttackCount_ %
			static_cast<int>(TankAttackPattern::Count));
	tankAttackTarget_ = {
		player.GetPosition().x + player.GetWidth() * 0.5f,
		player.GetPosition().y + player.GetHeight() * 0.5f
	};
	tankShotFired_ = false;
	tankShotsFired_ = 0;
	attackWindupFrames_ = TankFireDuration;
	attackWarningSoundRequested_ = true;
}

void Boss::UpdateAttack()
{
	// 中ボスの予約攻撃を、予告時間が終わった時点で実行する。
	if (isMineDashActive_)
	{
		if (--mineDashDropTimer_ <= 0)
		{
			SpawnMine();
			mineDashDropTimer_ = MineDashDropInterval;
		}
		return;
	}

	if (attackWindupFrames_ > 0)
	{
		--attackWindupFrames_;
		if (attackWindupFrames_ == 0)
		{
			if (pendingMineDashAttack_)
			{
				pendingMineDashAttack_ = false;
				isMineDashActive_ = true;
				SpawnMine();
				mineDashDropTimer_ = MineDashDropInterval;
				return;
			}
			if (pendingPincerAttack_)
			{
				SpawnPincerGrenadeVolley();
			}
			else if (pendingRicochetAttack_)
			{
				SpawnRicochetGrenadeVolley();
			}
			else if (pendingGrenadeAttack_)
			{
				SpawnGrenadeVolley();
			}
			else
			{
				SpawnMine();
			}

			++completedAttackCount_;
			attackCooldownFrames_ = AttackCooldownDuration;
			if (completedAttackCount_ % 2 == 0)
			{
				ReverseOwnGravity();
			}
		}
		return;
	}

	if (attackCooldownFrames_ > 0)
	{
		--attackCooldownFrames_;
		return;
	}

	const bool isEnraged = hitPoint_ * 2 <= maxHitPoint_;
	const int attackPatternCount = isEnraged ? 5 : 3;
	const int attackPattern = completedAttackCount_ % attackPatternCount;
	pendingGrenadeAttack_ = attackPattern == 1;
	pendingMineDashAttack_ = attackPattern == 2;
	pendingPincerAttack_ = attackPattern == 3;
	pendingRicochetAttack_ = attackPattern == 4;
	attackWindupFrames_ = AttackWindupDuration;
}

void Boss::SpawnMine()
{
	// 中ボスの足元へ機雷を設置する。
	const Vector2 center{
		position_.x + width_ * 0.5f,
		ownGravityDirection_ == GravityDirection::Down
			? position_.y + height_ - ProjectileSize * 0.5f
			: position_.y + ProjectileSize * 0.5f
	};
	SpawnProjectile(ProjectileType::Mine, center, {});
}

void Boss::SpawnGrenadeVolley()
{
	// 中ボスから複数方向へグレネードを放つ。
	grenadeSoundRequested_ = true;
	static constexpr float HorizontalSpeeds[5] =
		{ -6.0f, -3.0f, 0.0f, 3.0f, 6.0f };
	static constexpr float VerticalSpeeds[5] =
		{ 6.0f, 8.0f, 9.0f, 8.0f, 6.0f };
	const float verticalSign = ownGravityDirection_ == GravityDirection::Down
		? -1.0f : 1.0f;
	const Vector2 center{
		position_.x + width_ * 0.5f,
		position_.y + height_ * 0.5f
	};

	for (int i = 0; i < 5; ++i)
	{
		SpawnProjectile(ProjectileType::Grenade, center,
			{ HorizontalSpeeds[i], VerticalSpeeds[i] * verticalSign });
	}
}

void Boss::SpawnPincerGrenadeVolley()
{
	// 左右からプレイヤーを挟む軌道でグレネードを放つ。
	grenadeSoundRequested_ = true;
	static constexpr float HorizontalSpeeds[3] = { -5.0f, 0.0f, 5.0f };
	const Vector2 center{
		position_.x + width_ * 0.5f,
		position_.y + height_ * 0.5f
	};

	for (const float horizontalSpeed : HorizontalSpeeds)
	{
		SpawnProjectileWithGravity(ProjectileType::Grenade, center,
			{ horizontalSpeed, 2.0f }, GravityDirection::Down);
		SpawnProjectileWithGravity(ProjectileType::Grenade, center,
			{ horizontalSpeed, -2.0f }, GravityDirection::Up);
	}
}

void Boss::SpawnRicochetGrenadeVolley()
{
	// 壁で跳ね返るグレネードを複数生成する。
	grenadeSoundRequested_ = true;
	static constexpr float HorizontalSpeeds[3] = { 4.0f, 5.5f, 7.0f };
	static constexpr float VerticalSpeeds[3] = { 5.5f, 3.5f, 1.5f };
	const float horizontalSign = facing_ == EnemySpawnFacing::Right
		? 1.0f : -1.0f;
	const float verticalSign = ownGravityDirection_ == GravityDirection::Down
		? -1.0f : 1.0f;
	const Vector2 center{
		position_.x + width_ * 0.5f,
		position_.y + height_ * 0.5f
	};

	for (int i = 0; i < 3; ++i)
	{
		SpawnProjectile(ProjectileType::RicochetGrenade, center,
			{ HorizontalSpeeds[i] * horizontalSign,
				VerticalSpeeds[i] * verticalSign });
	}
}

void Boss::SpawnTankShell()
{
	// 大ボスの正面へ通常の砲弾を発射する。
	const float direction = facing_ == EnemySpawnFacing::Right ? 1.0f : -1.0f;
	const Vector2 center{
		direction > 0.0f ? position_.x + width_ + 10.0f : position_.x - 10.0f,
		ownGravityDirection_ == GravityDirection::Down
			? position_.y + 48.0f
			: position_.y + height_ - 48.0f
	};
	SpawnProjectile(ProjectileType::TankShell, center,
		{ direction * 11.0f, 0.0f });
}

void Boss::SpawnTankAimedShell()
{
	// 記録したプレイヤー位置へ向けて砲弾を発射する。
	const Vector2 center{
		position_.x + width_ * 0.5f,
		ownGravityDirection_ == GravityDirection::Down
			? position_.y - 4.0f
			: position_.y + height_ + 4.0f
	};
	float directionX = tankAttackTarget_.x - center.x;
	float directionY = tankAttackTarget_.y - center.y;
	const float length = std::sqrt(
		directionX * directionX + directionY * directionY);
	if (length > 0.01f)
	{
		directionX /= length;
		directionY /= length;
	}
	else
	{
		directionY = ownGravityDirection_ == GravityDirection::Down
			? -1.0f : 1.0f;
	}
	SpawnProjectile(ProjectileType::TankShell, center,
		{ directionX * 10.0f, directionY * 10.0f });
}

void Boss::SpawnTankGrenadeVolley()
{
	// 大ボスから放射状にグレネードを発射する。
	grenadeSoundRequested_ = true;
	static constexpr float HorizontalSpeeds[5] =
		{ 3.0f, 4.2f, 5.4f, 6.6f, 7.8f };
	static constexpr float VerticalSpeeds[5] =
		{ -5.0f, -6.2f, -7.4f, -8.6f, -9.8f };
	const float direction = facing_ == EnemySpawnFacing::Right ? 1.0f : -1.0f;
	const Vector2 center{
		direction > 0.0f ? position_.x + width_ + 4.0f : position_.x - 4.0f,
		ownGravityDirection_ == GravityDirection::Down
			? position_.y + 42.0f
			: position_.y + height_ - 42.0f
	};
	const float verticalSign = ownGravityDirection_ == GravityDirection::Down
		? 1.0f : -1.0f;
	const int grenadeCount = hitPoint_ * 2 <= maxHitPoint_ ? 5 : 4;
	for (int i = 0; i < grenadeCount; ++i)
	{
		SpawnProjectile(ProjectileType::Grenade, center,
			{ HorizontalSpeeds[i] * direction,
				VerticalSpeeds[i] * verticalSign });
	}
}

void Boss::SpawnTankMines()
{
	// 大ボスの周囲へ複数の機雷を設置する。
	const int mineCount = hitPoint_ * 2 <= maxHitPoint_ ? 3 : 2;
	for (int i = 0; i < mineCount; ++i)
	{
		const float ratio = static_cast<float>(i + 1) /
			static_cast<float>(mineCount + 1);
		const Vector2 center{
			position_.x + width_ * ratio,
			ownGravityDirection_ == GravityDirection::Down
				? position_.y + height_ - ProjectileSize * 0.5f
				: position_.y + ProjectileSize * 0.5f
		};
		SpawnProjectile(ProjectileType::Mine, center, {});
	}
}

void Boss::SpawnProjectile(ProjectileType type,
	const Vector2& center, const Vector2& velocity)
{
	SpawnProjectileWithGravity(type, center, velocity, ownGravityDirection_);
}

void Boss::SpawnProjectileWithGravity(ProjectileType type,
	const Vector2& center, const Vector2& velocity,
	GravityDirection gravityDirection)
{
	// 固定長配列の空き要素を初期化し、攻撃ごとの重力方向も保存する。
	for (Projectile& projectile : projectiles_)
	{
		if (projectile.isActive)
		{
			continue;
		}

		projectile = {};
		projectile.position = {
			center.x - ProjectileSize * 0.5f,
			center.y - ProjectileSize * 0.5f
		};
		projectile.velocity = velocity;
		projectile.type = type;
		projectile.gravityDirection = gravityDirection;
		projectile.timer = type == ProjectileType::Mine ? 360 :
			(type == ProjectileType::TankShell ? 240 : 480);
		projectile.remainingBounces =
			type == ProjectileType::RicochetGrenade ? 2 : 0;
		projectile.isActive = true;
		return;
	}
}

void Boss::UpdateProjectiles(const StageData& stageData)
{
	// 種類ごとに移動、重力、地形反射、爆発への切り替えを処理する。
	for (Projectile& projectile : projectiles_)
	{
		if (!projectile.isActive)
		{
			continue;
		}
		if (projectile.type == ProjectileType::Explosion)
		{
			if (--projectile.timer <= 0)
			{
				projectile.isActive = false;
			}
			continue;
		}
		if (projectile.type == ProjectileType::Mine)
		{
			if (--projectile.timer <= 0)
			{
				ExplodeProjectile(projectile);
			}
			continue;
		}
		if (projectile.type == ProjectileType::TankShell)
		{
			const float nextX = projectile.position.x + projectile.velocity.x;
			const float nextY = projectile.position.y + projectile.velocity.y;
			if (nextX < 0.0f || nextY < 0.0f ||
				nextX + ProjectileSize > StageData::StagePixelWidth ||
				nextY + ProjectileSize > stageData.GetPixelHeight() ||
				RectOverlapsSolid(stageData, nextX, nextY,
					ProjectileSize, ProjectileSize))
			{
				ExplodeProjectile(projectile);
				continue;
			}

			projectile.position.x = nextX;
			projectile.position.y = nextY;
			if (--projectile.timer <= 0)
			{
				ExplodeProjectile(projectile);
			}
			continue;
		}

		if (!projectile.isRolling)
		{
			const float gravitySign =
				projectile.gravityDirection == GravityDirection::Down
				? 1.0f : -1.0f;
			projectile.velocity.y += ProjectileGravity * gravitySign;
			projectile.velocity.y = (std::max)(-MaximumProjectileFallSpeed,
				(std::min)(MaximumProjectileFallSpeed, projectile.velocity.y));
		}

		const float nextX = projectile.position.x + projectile.velocity.x;
		const bool hitHorizontalSurface = nextX < 0.0f ||
			nextX + ProjectileSize > StageData::StagePixelWidth ||
			RectOverlapsSolid(stageData, nextX, projectile.position.y,
				ProjectileSize, ProjectileSize);
		if (hitHorizontalSurface)
		{
			if (projectile.type == ProjectileType::RicochetGrenade &&
				projectile.remainingBounces > 0)
			{
				projectile.velocity.x = -projectile.velocity.x;
				--projectile.remainingBounces;
			}
			else
			{
				ExplodeProjectile(projectile);
				continue;
			}
		}
		else
		{
			projectile.position.x = nextX;
		}

		if (projectile.isRolling)
		{
			if (!HasProjectileSupport(projectile, stageData))
			{
				projectile.isRolling = false;
			}
		}
		else
		{
			const float nextY = projectile.position.y + projectile.velocity.y;
			if (RectOverlapsSolid(stageData, projectile.position.x, nextY,
				ProjectileSize, ProjectileSize))
			{
				if (projectile.type == ProjectileType::RicochetGrenade)
				{
					if (projectile.remainingBounces <= 0)
					{
						ExplodeProjectile(projectile);
						continue;
					}
					if (projectile.velocity.y >= 0.0f)
					{
						const int row = static_cast<int>(
							(nextY + ProjectileSize - 1.0f) /
							StageData::TileSize);
						projectile.position.y =
							static_cast<float>(row * StageData::TileSize) -
							ProjectileSize;
					}
					else
					{
						const int row = static_cast<int>(
							std::floor(nextY / StageData::TileSize));
						projectile.position.y = static_cast<float>(
							(row + 1) * StageData::TileSize);
					}
					projectile.velocity.y = -projectile.velocity.y * 0.8f;
					--projectile.remainingBounces;
					continue;
				}

				const bool movingWithGravity =
					(projectile.gravityDirection == GravityDirection::Down &&
						projectile.velocity.y > 0.0f) ||
					(projectile.gravityDirection == GravityDirection::Up &&
						projectile.velocity.y < 0.0f);
				if (!movingWithGravity)
				{
					ExplodeProjectile(projectile);
					continue;
				}

				if (projectile.gravityDirection == GravityDirection::Down)
				{
					const int row = static_cast<int>(
						(nextY + ProjectileSize - 1.0f) / StageData::TileSize);
					projectile.position.y =
						static_cast<float>(row * StageData::TileSize) - ProjectileSize;
				}
				else
				{
					const int row = static_cast<int>(
						std::floor(nextY / StageData::TileSize));
					projectile.position.y =
						static_cast<float>((row + 1) * StageData::TileSize);
				}
				projectile.velocity.y = 0.0f;
				projectile.isRolling = true;
				if (std::abs(projectile.velocity.x) < 1.0f)
				{
					projectile.velocity.x = facing_ == EnemySpawnFacing::Right
						? 3.0f : -3.0f;
				}
			}
			else
			{
				projectile.position.y = nextY;
			}
		}

		if (--projectile.timer <= 0 ||
			projectile.position.y < -ExplosionSize ||
			projectile.position.y > stageData.GetPixelHeight() + ExplosionSize)
		{
			ExplodeProjectile(projectile);
		}
	}
}

void Boss::ExplodeProjectile(Projectile& projectile)
{
	// 元の攻撃を爆発判定へ置き換え、描画用エフェクトを要求する。
	if (!projectile.isActive || projectile.type == ProjectileType::Explosion)
	{
		return;
	}
	const Vector2 center{
		projectile.position.x + ProjectileSize * 0.5f,
		projectile.position.y + ProjectileSize * 0.5f
	};
	projectile.position = {
		center.x - ExplosionSize * 0.5f,
		center.y - ExplosionSize * 0.5f
	};
	projectile.velocity = {};
	projectile.type = ProjectileType::Explosion;
	projectile.timer = 18;
	projectile.isRolling = false;
	projectile.hasHitPlayer = false;
	if (pendingExplosionEffectCount_ <
		static_cast<int>(pendingExplosionEffects_.size()))
	{
		pendingExplosionEffects_[pendingExplosionEffectCount_++] =
			projectile.position;
	}
}

void Boss::ReverseOwnGravity()
{
	// プレイヤーの重力とは独立して、ボス自身の上下方向を反転する。
	ownGravityDirection_ = ownGravityDirection_ == GravityDirection::Down
		? GravityDirection::Up : GravityDirection::Down;
	velocity_.y = ownGravityDirection_ == GravityDirection::Down ? 4.0f : -4.0f;
}

void Boss::ClearProjectiles()
{
	// リトライや戦闘終了時に、画面に残った攻撃を全て消す。
	projectiles_.fill(Projectile{});
	pendingExplosionEffects_.fill(Vector2{});
	pendingExplosionEffectCount_ = 0;
}

bool Boss::RectOverlapsSolid(const StageData& stageData,
	float left, float top, float width, float height) const
{
	const int firstColumn = static_cast<int>(std::floor(left / StageData::TileSize));
	const int lastColumn = static_cast<int>(std::floor(
		(left + width - 0.01f) / StageData::TileSize));
	const int firstRow = static_cast<int>(std::floor(top / StageData::TileSize));
	const int lastRow = static_cast<int>(std::floor(
		(top + height - 0.01f) / StageData::TileSize));
	for (int row = firstRow; row <= lastRow; ++row)
	{
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (IsSolidAt(stageData, column, row))
			{
				return true;
			}
		}
	}
	return false;
}

bool Boss::HasProjectileSupport(const Projectile& projectile,
	const StageData& stageData) const
{
	const float probeY = projectile.gravityDirection == GravityDirection::Down
		? projectile.position.y + ProjectileSize + 1.0f
		: projectile.position.y - 1.0f;
	const int row = static_cast<int>(std::floor(probeY / StageData::TileSize));
	const int firstColumn = static_cast<int>(std::floor(
		projectile.position.x / StageData::TileSize));
	const int lastColumn = static_cast<int>(std::floor(
		(projectile.position.x + ProjectileSize - 0.01f) /
		StageData::TileSize));
	for (int column = firstColumn; column <= lastColumn; ++column)
	{
		if (IsSolidAt(stageData, column, row))
		{
			return true;
		}
	}
	return false;
}

void Boss::Draw() const
{
	// ボス本体、攻撃予告、全飛び道具の順に描画する。
	if (!isActive_)
	{
		return;
	}
	if (hitFlashFrames_ > 0 && (hitFlashFrames_ / 2) % 2 == 0)
	{
		DrawAttackWarning();
		DrawProjectiles();
		return;
	}

	bool drewSprite = false;
	if (type_ == BossType::MidBoss &&
		(walkImageHandle_ != -1 || idleImageHandle_ != -1))
	{
		constexpr int frameSize = 128;
		constexpr int framesPerRow = 4;
		const bool useIdle = attackWindupFrames_ > 0 && idleImageHandle_ != -1;
		const int frameCount = useIdle ? 8 : 6;
		const int imageHandle = useIdle ? idleImageHandle_ : walkImageHandle_;
		const int frameDuration = isMineDashActive_ ? 3 : 8;
		const int frame = (animationTimer_ / frameDuration) % frameCount;
		const int sourceLeft = (frame % framesPerRow) * frameSize;
		const int sourceTop = (frame / framesPerRow) * frameSize;
		const int left = static_cast<int>(position_.x);
		const int top = static_cast<int>(position_.y);
		const int right = static_cast<int>(position_.x + width_);
		const int bottom = static_cast<int>(position_.y + height_);
		const bool flipX = facing_ == EnemySpawnFacing::Left;
		const bool flipY = ownGravityDirection_ == GravityDirection::Up;
		DrawRectModiGraph(
			flipX ? right : left, flipY ? bottom : top,
			flipX ? left : right, flipY ? bottom : top,
			flipX ? left : right, flipY ? top : bottom,
			flipX ? right : left, flipY ? top : bottom,
			sourceLeft, sourceTop, frameSize, frameSize,
			imageHandle, TRUE);
		drewSprite = true;
	}
	else if (type_ == BossType::Boss &&
		(tankMoveImageHandle_ != -1 || tankFireImageHandle_ != -1))
	{
		constexpr int frameSize = 256;
		constexpr int framesPerRow = 4;
		constexpr int sourceOffsetX = 20;
		constexpr int sourceOffsetY = 120;
		constexpr int sourceWidth = 204;
		constexpr int sourceHeight = 136;
		const bool isFiring = attackWindupFrames_ > 0 &&
			tankFireImageHandle_ != -1;
		const int imageHandle = isFiring
			? tankFireImageHandle_ : tankMoveImageHandle_;
		const int elapsedFireFrames = TankFireDuration - attackWindupFrames_;
		const int frame = isFiring
			? (std::min)(TankFireFrameCount - 1,
				(std::max)(0, elapsedFireFrames / TankAnimationFrameDuration))
			: (animationTimer_ / TankAnimationFrameDuration) % TankMoveFrameCount;
		const int sourceLeft = (frame % framesPerRow) * frameSize + sourceOffsetX;
		const int sourceTop = (frame / framesPerRow) * frameSize + sourceOffsetY;
		const int drawLeft = static_cast<int>(position_.x) - 12;
		const int drawTop = static_cast<int>(position_.y) - 16;
		const int drawRight = drawLeft + sourceWidth;
		const int drawBottom = drawTop + sourceHeight;
		const bool flipX = facing_ == EnemySpawnFacing::Left;
		const bool flipY = ownGravityDirection_ == GravityDirection::Up;

		DrawRectModiGraph(
			flipX ? drawRight : drawLeft, flipY ? drawBottom : drawTop,
			flipX ? drawLeft : drawRight, flipY ? drawBottom : drawTop,
			flipX ? drawLeft : drawRight, flipY ? drawTop : drawBottom,
			flipX ? drawRight : drawLeft, flipY ? drawTop : drawBottom,
			sourceLeft, sourceTop, sourceWidth, sourceHeight,
			imageHandle, TRUE);
		drewSprite = true;
	}

	if (!drewSprite)
	{
		const int color = type_ == BossType::MidBoss
			? GetColor(180, 80, 255) : GetColor(220, 40, 80);
		DrawBox(static_cast<int>(position_.x), static_cast<int>(position_.y),
			static_cast<int>(position_.x + width_),
			static_cast<int>(position_.y + height_), color, TRUE);
	}

	DrawAttackWarning();
	DrawProjectiles();
}

void Boss::DrawAttackWarning() const
{
	if (attackWindupFrames_ <= 0 ||
		(type_ == BossType::Boss && tankShotFired_))
	{
		return;
	}

	const int centerX = static_cast<int>(position_.x + width_ * 0.5f);
	const bool flipY = ownGravityDirection_ == GravityDirection::Up;
	if (attackWarningImageHandle_ == -1)
	{
		const int warningY = flipY
			? static_cast<int>(position_.y + height_) + 20
			: static_cast<int>(position_.y) - 20;
		DrawCircle(centerX, warningY, 7, GetColor(255, 70, 70), TRUE);
		return;
	}

	const int warningDuration = type_ == BossType::Boss
		? TankFireFrame * TankAnimationFrameDuration
		: AttackWindupDuration;
	const int elapsedFrames = type_ == BossType::Boss
		? TankFireDuration - attackWindupFrames_
		: warningDuration - attackWindupFrames_;
	const int frame = (elapsedFrames * AttackWarningFrameCount) /
		warningDuration;
	const int sourceLeft = frame * AttackWarningFrameSize;
	const int drawLeft = centerX - AttackWarningFrameSize / 2;
	const int drawRight = drawLeft + AttackWarningFrameSize;
	const int drawTop = flipY
		? static_cast<int>(position_.y + height_) - 8
		: static_cast<int>(position_.y) - AttackWarningFrameSize + 8;
	const int drawBottom = drawTop + AttackWarningFrameSize;

	DrawRectModiGraph(
		drawLeft, flipY ? drawBottom : drawTop,
		drawRight, flipY ? drawBottom : drawTop,
		drawRight, flipY ? drawTop : drawBottom,
		drawLeft, flipY ? drawTop : drawBottom,
		sourceLeft, 0, AttackWarningFrameSize, AttackWarningFrameSize,
		attackWarningImageHandle_, TRUE);
}

void Boss::DrawProjectiles() const
{
	for (const Projectile& projectile : projectiles_)
	{
		if (!projectile.isActive)
		{
			continue;
		}

		const int centerX = static_cast<int>(projectile.position.x +
			(projectile.type == ProjectileType::Explosion
				? ExplosionSize : ProjectileSize) * 0.5f);
		const int centerY = static_cast<int>(projectile.position.y +
			(projectile.type == ProjectileType::Explosion
				? ExplosionSize : ProjectileSize) * 0.5f);
		if (projectile.type == ProjectileType::Explosion)
		{
			continue;
		}
		if (projectile.type == ProjectileType::Mine && mineImageHandle_ != -1)
		{
			DrawGraph(static_cast<int>(projectile.position.x),
				static_cast<int>(projectile.position.y), mineImageHandle_, TRUE);
			continue;
		}
		if ((projectile.type == ProjectileType::Grenade ||
			projectile.type == ProjectileType::RicochetGrenade) &&
			grenadeImageHandle_ != -1)
		{
			DrawRotaGraph(centerX, centerY, 1.0,
				animationTimer_ * 0.12, grenadeImageHandle_, TRUE);
			continue;
		}
		if (projectile.type == ProjectileType::TankShell)
		{
			const float speed = std::sqrt(
				projectile.velocity.x * projectile.velocity.x +
				projectile.velocity.y * projectile.velocity.y);
			const float directionX = speed > 0.01f
				? projectile.velocity.x / speed : 1.0f;
			const float directionY = speed > 0.01f
				? projectile.velocity.y / speed : 0.0f;
			DrawLine(centerX - static_cast<int>(directionX * 14.0f),
				centerY - static_cast<int>(directionY * 14.0f),
				centerX + static_cast<int>(directionX * 8.0f),
				centerY + static_cast<int>(directionY * 8.0f),
				GetColor(255, 120, 30), 8);
			DrawCircle(centerX + static_cast<int>(directionX * 7.0f),
				centerY + static_cast<int>(directionY * 7.0f), 6,
				GetColor(255, 235, 100), TRUE);
			continue;
		}

		DrawCircle(centerX, centerY, 12,
			projectile.type == ProjectileType::Mine
				? GetColor(120, 170, 190) : GetColor(255, 140, 20), TRUE);
	}
}

bool Boss::CheckProjectileCollision(
	const Player& player, float& damageSourceCenterX)
{
	// ボス本体と有効な攻撃を順に調べ、最初の接触だけを返す。
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();
	const float playerCenterX = (playerLeft + playerRight) * 0.5f;
	const float playerCenterY = (playerTop + playerBottom) * 0.5f;

	for (Projectile& projectile : projectiles_)
	{
		if (!projectile.isActive)
		{
			continue;
		}
		if (projectile.type == ProjectileType::Mine)
		{
			const float mineCenterX = projectile.position.x + ProjectileSize * 0.5f;
			const float mineCenterY = projectile.position.y + ProjectileSize * 0.5f;
			const float deltaX = playerCenterX - mineCenterX;
			const float deltaY = playerCenterY - mineCenterY;
			if (deltaX * deltaX + deltaY * deltaY <= 90.0f * 90.0f)
			{
				ExplodeProjectile(projectile);
			}
		}

		const float size = projectile.type == ProjectileType::Explosion
			? ExplosionSize : ProjectileSize;
		const float projectileLeft = projectile.position.x;
		const float projectileTop = projectile.position.y;
		const float projectileRight = projectileLeft + size;
		const float projectileBottom = projectileTop + size;
		const bool overlaps = playerLeft < projectileRight &&
			playerRight > projectileLeft && playerTop < projectileBottom &&
			playerBottom > projectileTop;
		if (!overlaps || (projectile.type == ProjectileType::Explosion &&
			projectile.hasHitPlayer))
		{
			continue;
		}

		damageSourceCenterX = (projectileLeft + projectileRight) * 0.5f;
		if (projectile.type == ProjectileType::Grenade ||
			projectile.type == ProjectileType::RicochetGrenade ||
			projectile.type == ProjectileType::TankShell)
		{
			ExplodeProjectile(projectile);
		}
		projectile.hasHitPlayer = true;
		return true;
	}

	return false;
}

void Boss::ResetAttackProjectiles()
{
	// チェックポイントからの再開に備え、攻撃と戦闘開始時のHPを戻す。
	// チェックポイント復活では、攻撃状態と一緒に配置時のHPへ戻す。
	if (isInitialized_)
	{
		hitPoint_ = maxHitPoint_;
		hitFlashFrames_ = 0;
		isDefeated_ = false;
		if (hasEncounterStarted_)
		{
			isActive_ = true;
		}
	}

	ClearProjectiles();
	attackWindupFrames_ = 0;
	attackCooldownFrames_ = 90;
	pendingGrenadeAttack_ = false;
	pendingMineDashAttack_ = false;
	pendingPincerAttack_ = false;
	pendingRicochetAttack_ = false;
	grenadeSoundRequested_ = false;
	attackWarningSoundRequested_ = false;
	isMineDashActive_ = false;
	mineDashDropTimer_ = 0;
	tankAttackPattern_ = TankAttackPattern::Cannon;
	tankShotFired_ = false;
	tankShotsFired_ = 0;
	tankGravityMismatchFrames_ = 0;
	tankGravityFlipCooldownFrames_ = 0;
	tankAttackTarget_ = {};
}

bool Boss::ConsumeExplosionEffect(Vector2& effectPosition)
{
	if (pendingExplosionEffectCount_ <= 0)
	{
		return false;
	}
	effectPosition =
		pendingExplosionEffects_[--pendingExplosionEffectCount_];
	pendingExplosionEffects_[pendingExplosionEffectCount_] = {};
	return true;
}

bool Boss::ConsumeGrenadeSoundRequest()
{
	const bool requested = grenadeSoundRequested_;
	grenadeSoundRequested_ = false;
	return requested;
}

bool Boss::ConsumeAttackWarningSoundRequest()
{
	const bool requested = attackWarningSoundRequested_;
	attackWarningSoundRequested_ = false;
	return requested;
}

void Boss::TakeDamage()
{
	// 連続被弾を許可しつつ、視覚上のヒット点滅を開始する。
	if (!isActive_)
	{
		return;
	}
	hitFlashFrames_ = HitFlashDuration;
	if (--hitPoint_ <= 0)
	{
		isActive_ = false;
		isDefeated_ = true;
		velocity_ = {};
		ClearProjectiles();
		return;
	}
}

void Boss::Deactivate()
{
	// ステージからボスを外し、次の初期化まで全状態と攻撃を停止する。
	isInitialized_ = false;
	hasEncounterStarted_ = false;
	isActive_ = false;
	isDefeated_ = false;
	velocity_ = {};
	hitFlashFrames_ = 0;
	pendingGrenadeAttack_ = false;
	pendingMineDashAttack_ = false;
	pendingPincerAttack_ = false;
	pendingRicochetAttack_ = false;
	grenadeSoundRequested_ = false;
	attackWarningSoundRequested_ = false;
	isMineDashActive_ = false;
	mineDashDropTimer_ = 0;
	tankAttackPattern_ = TankAttackPattern::Cannon;
	tankShotFired_ = false;
	tankShotsFired_ = 0;
	tankGravityMismatchFrames_ = 0;
	tankGravityFlipCooldownFrames_ = 0;
	tankAttackTarget_ = {};
	ClearProjectiles();
}

void Boss::OnGravityReversed()
{
	// ボスは独自の重力方向を持つため、プレイヤー側の反転では変更しない。
}

bool Boss::IsActive() const { return isActive_; }
bool Boss::IsDefeated() const { return isDefeated_; }
bool Boss::HasEncounterStarted() const { return hasEncounterStarted_; }
const Vector2& Boss::GetPosition() const { return position_; }
float Boss::GetWidth() const { return width_; }
float Boss::GetHeight() const { return height_; }
int Boss::GetHitPoint() const { return hitPoint_; }

bool Boss::IsSolidAt(const StageData& stageData, int column, int row) const
{
	return column < 0 || column >= StageData::TileColumns ||
		row < 0 || row >= stageData.tileRowCount ||
		StageData::IsSolidTile(stageData.tiles[row][column]);
}

void Boss::ApplyGravity(const GravitySystem& gravitySystem)
{
	// 渡された重力加速度を縦速度へ加え、位置へ反映する。
	velocity_.y += gravitySystem.GetAcceleration();
	position_.y += velocity_.y;
}

void Boss::ResolveVerticalCollision(const GravitySystem& gravitySystem)
{
	// 接地面を越えないようにY座標を補正し、縦速度を止める。
	if (gravitySystem.GetDirection() == GravityDirection::Down)
	{
		const float floorTop = static_cast<float>(GameConfig::FloorY) - height_;
		if (position_.y >= floorTop)
		{
			position_.y = floorTop;
			velocity_.y = 0.0f;
		}
		return;
	}
	const float ceilingBottom = static_cast<float>(GameConfig::CeilingY);
	if (position_.y <= ceilingBottom)
	{
		position_.y = ceilingBottom;
		velocity_.y = 0.0f;
	}
}
