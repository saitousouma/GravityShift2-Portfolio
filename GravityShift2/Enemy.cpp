#include "Enemy.h"

#include "EnemyBulletManager.h"
#include "EnemyTerrainCollision.h"
#include "GameConfig.h"
#include "Player.h"
#include "StageData.h"
#include "DxLib.h"

// 敵を指定した位置に配置する。
void Enemy::Initialize(
	const Vector2& position,
	int hitPoint,
	GravityDirection gravityDirection,
	EnemyType type,
	bool isAffectedByGravityReversal,
	float horizontalDirection
)
{
	position_ = position;
	previousPosition_ = position_;

	velocity_.x = 0.0f;
	velocity_.y = 0.0f;

	gravityDirection_ = gravityDirection;
	type_ = type;
	isAffectedByGravityReversal_ = isAffectedByGravityReversal;

	hitPoint_ = hitPoint;
	damageCooldownFrames_ = 0;
	shootCooldownFrames_ = 0;
	walkAnimationFrame_ = 0;
	walkAnimationTimer_ = 0;
	shooterAnimationFrame_ = 0;
	shooterAnimationTimer_ = 0;
	isShooterAnimating_ = false;
	chargerAnimationFrame_ = 0;
	chargerAnimationTimer_ = 0;
	isChargerActivated_ = false;
	isChargerStopped_ = false;
	horizontalDirection_ = horizontalDirection >= 0.0f ? 1.0f : -1.0f;

	width_ = GetWidthByType(type_);
	height_ = GetHeightByType(type_);

	// 敵の種類に応じて基本性能を設定する。
	if (type_ == EnemyType::Normal)
	{
		moveSpeed_ = 2.0f;
	}
	else if (type_ == EnemyType::Heavy)
	{
		moveSpeed_ = 1.2f;
	}
	else if (type_ == EnemyType::Shooter || type_ == EnemyType::UpShooter)
	{
		// 射撃敵はその場からあまり動かないよう、通常敵より遅くする。
		moveSpeed_ = 0.6f;
		shootCooldownFrames_ = ShooterWarningFrames;
	}
	else if (type_ == EnemyType::Charger)
	{
		moveSpeed_ = ChargerMoveSpeed;
		isAffectedByGravityReversal_ = false;
	}
	else if (type_ == EnemyType::Mortar)
	{
		moveSpeed_ = 0.0f;
		isAffectedByGravityReversal_ = false;
		shootCooldownFrames_ = MortarAttackCooldown;
	}

	isActive_ = true;
}

void Enemy::SetImageHandles(
	int normalImageHandle,
	int heavyImageHandle,
	int shooterImageHandle,
	int chargerImageHandle,
	int mortarImageHandle)
{
	normalImageHandle_ = normalImageHandle;
	heavyImageHandle_ = heavyImageHandle;
	shooterImageHandle_ = shooterImageHandle;
	chargerImageHandle_ = chargerImageHandle;
	mortarImageHandle_ = mortarImageHandle;
}

void Enemy::Update(
	const GravitySystem& gravitySystem,
	const StageData& currentStage,
	const Player& player,
	EnemyBulletManager& enemyBulletManager)
{
	if (!isActive_)
	{
		return;
	}

	if (damageCooldownFrames_ > 0)
	{
		--damageCooldownFrames_;
	}

	previousPosition_ = position_;

	if (type_ == EnemyType::Mortar)
	{
		UpdateMortar(gravitySystem, enemyBulletManager);
		return;
	}
	if (type_ == EnemyType::Charger)
	{
		UpdateCharger(player);
	}
	else if (type_ == EnemyType::Shooter || type_ == EnemyType::UpShooter)
	{
		UpdateShooterAttack(player, enemyBulletManager);
	}
	else
	{
		UpdateHorizontalMovement();

		if (type_ == EnemyType::Normal || type_ == EnemyType::Heavy)
		{
			++walkAnimationTimer_;
			if (walkAnimationTimer_ >= WalkAnimationFrameDuration)
			{
				walkAnimationTimer_ = 0;
				walkAnimationFrame_ =
					(walkAnimationFrame_ + 1) % WalkAnimationFrameCount;
			}
		}
	}

	ResolveTileHorizontalCollision(currentStage);
	if (type_ == EnemyType::Charger)
	{
		return;
	}
	ApplyGravity(gravitySystem);
	ResolveVerticalCollision(gravitySystem, currentStage);
	CheckFallLoop(currentStage);
}

void Enemy::ResolveTileHorizontalCollision(const StageData& currentStage)
{
	const float movementX = position_.x - previousPosition_.x;
	float resolvedX = position_.x;
	if (!EnemyTerrainCollision::ResolveHorizontal(currentStage,
		previousPosition_, position_, width_, height_, resolvedX))
	{
		return;
	}

	position_.x = resolvedX;
	if (type_ == EnemyType::Charger)
	{
		isChargerStopped_ = true;
		chargerAnimationFrame_ = 0;
	}
	else
	{
		horizontalDirection_ = movementX > 0.0f ? -1.0f : 1.0f;
	}
	knockbackVelocityX_ = 0.0f;
}

void Enemy::OnGravityReversed()
{
	if (type_ == EnemyType::Charger || type_ == EnemyType::Mortar ||
		!isAffectedByGravityReversal_)
	{
		return;
	}

	velocity_.y = 0.0f;

	if (gravityDirection_ == GravityDirection::Down)
	{
		gravityDirection_ = GravityDirection::Up;
	}
	else
	{
		gravityDirection_ = GravityDirection::Down;
	}
}

void Enemy::UpdateHorizontalMovement()
{
	position_.x += moveSpeed_ * horizontalDirection_;
	position_.x += knockbackVelocityX_;

	// ノックバックは毎フレーム少しずつ弱める。
	knockbackVelocityX_ *= 0.85f;

	if (knockbackVelocityX_ > -0.1f && knockbackVelocityX_ < 0.1f)
	{
		knockbackVelocityX_ = 0.0f;
	}

	if (position_.x <= 0.0f)
	{
		position_.x = 0.0f;
		horizontalDirection_ = 1.0f;
		return;
	}

	const float rightLimit =
		static_cast<float>(StageData::StagePixelWidth) - width_;

	if (position_.x >= rightLimit)
	{
		position_.x = rightLimit;
		horizontalDirection_ = -1.0f;
	}
}

void Enemy::UpdateCharger(const Player& player)
{
	if (isChargerStopped_)
	{
		return;
	}

	if (!isChargerActivated_)
	{
		if (!CanDetectPlayer(player, ChargerDetectionRange))
		{
			return;
		}
		isChargerActivated_ = true;
	}

	position_.x += ChargerMoveSpeed * horizontalDirection_;
	++chargerAnimationTimer_;
	if (chargerAnimationTimer_ >= ChargerAnimationFrameDuration)
	{
		chargerAnimationTimer_ = 0;
		chargerAnimationFrame_ =
			(chargerAnimationFrame_ + 1) % ChargerAnimationFrameCount;
	}

	const float rightLimit =
		static_cast<float>(StageData::StagePixelWidth) - width_;
	if (position_.x <= 0.0f)
	{
		position_.x = 0.0f;
		isChargerStopped_ = true;
	}
	else if (position_.x >= rightLimit)
	{
		position_.x = rightLimit;
		isChargerStopped_ = true;
	}
}

void Enemy::UpdateMortar(
	const GravitySystem& gravitySystem,
	EnemyBulletManager& enemyBulletManager)
{
	if (shootCooldownFrames_ > 0)
	{
		--shootCooldownFrames_;
		return;
	}

	enemyBulletManager.ShootMortar(GetCenterPosition(),
		gravitySystem.GetDirection() == GravityDirection::Down);
	shootCooldownFrames_ = MortarAttackCooldown;
}

// 射撃敵がプレイヤーを見つけたら、その場で止まって弾を撃つ。
void Enemy::UpdateShooterAttack(
	const Player& player,
	EnemyBulletManager& enemyBulletManager)
{
	const Vector2 enemyCenter = GetCenterPosition();
	const Vector2 playerCenter{
		player.GetPosition().x + player.GetWidth() * 0.5f,
		player.GetPosition().y + player.GetHeight() * 0.5f
	};

	if (playerCenter.x < enemyCenter.x)
	{
		horizontalDirection_ = -1.0f;
	}
	else
	{
		horizontalDirection_ = 1.0f;
	}

	if (!CanDetectPlayer(player, ShooterDetectionRange))
	{
		shooterAnimationFrame_ = 0;
		shooterAnimationTimer_ = 0;
		isShooterAnimating_ = false;
		UpdateHorizontalMovement();
		return;
	}

	// 射撃準備中は8コマを順に進め、7コマ目へ入った瞬間に弾を撃つ。
	if (isShooterAnimating_)
	{
		++shooterAnimationTimer_;
		if (shooterAnimationTimer_ < ShooterAnimationFrameDuration)
		{
			return;
		}

		shooterAnimationTimer_ = 0;
		++shooterAnimationFrame_;

		if (shooterAnimationFrame_ == ShooterFireFrame)
		{
			if (type_ == EnemyType::UpShooter)
			{
				enemyBulletManager.ShootVerticalLoop(enemyCenter,
					gravityDirection_ == GravityDirection::Up);
			}
			else
			{
				enemyBulletManager.Shoot(enemyCenter, playerCenter);
			}
			shootCooldownFrames_ = ShooterAttackInterval;
		}

		if (shooterAnimationFrame_ >= ShooterAnimationFrameCount)
		{
			shooterAnimationFrame_ = 0;
			isShooterAnimating_ = false;
		}

		return;
	}

	// 検知中は移動せず、クールダウンが終わったら射撃アニメーションを始める。
	if (shootCooldownFrames_ > 0)
	{
		--shootCooldownFrames_;
		return;
	}

	shooterAnimationFrame_ = 0;
	shooterAnimationTimer_ = 0;
	isShooterAnimating_ = true;
}

// プレイヤーとの距離を使って、射撃敵が反応するか決める。
bool Enemy::CanDetectPlayer(const Player& player, float detectionRange) const
{
	const Vector2 enemyCenter = GetCenterPosition();
	const Vector2 playerCenter{
		player.GetPosition().x + player.GetWidth() * 0.5f,
		player.GetPosition().y + player.GetHeight() * 0.5f
	};

	const float diffX = playerCenter.x - enemyCenter.x;
	const float diffY = playerCenter.y - enemyCenter.y;
	const float distanceSquared = diffX * diffX + diffY * diffY;

	return distanceSquared <= detectionRange * detectionRange;
}

Vector2 Enemy::GetCenterPosition() const
{
	return Vector2{
		position_.x + width_ * 0.5f,
		position_.y + height_ * 0.5f
	};
}

// 現在の重力方向に合わせて落下させる。
void Enemy::ApplyGravity(const GravitySystem& gravitySystem)
{
	float acceleration = gravitySystem.GetAcceleration();

	if (acceleration < 0.0f)
	{
		acceleration *= -1.0f;
	}
	acceleration *= FallAccelerationScale;

	if (gravityDirection_ == GravityDirection::Down)
	{
		velocity_.y += acceleration;

		if (velocity_.y > maxFallSpeed_)
		{
			velocity_.y = maxFallSpeed_;
		}
	}
	else
	{
		velocity_.y -= acceleration;

		if (velocity_.y < -maxFallSpeed_)
		{
			velocity_.y = -maxFallSpeed_;
		}
	}

	position_.y += velocity_.y;
}

// 重力が下なら床、上なら天井との衝突を解決する。
void Enemy::ResolveVerticalCollision(
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	// 先にタイル足場との接触を見る。
	// 浮いている足場に触れた場合はここで接地が完了する。
	if (ResolveTileVerticalCollision(currentStage))
	{
		return;
	}

	// 穴の上では床にも天井にも接地させず、そのまま落下させる。
	if (IsOverFallHole(currentStage))
	{
		return;
	}

	if (gravityDirection_ == GravityDirection::Down)
	{
		const float floorTop =
			static_cast<float>(currentStage.GetPixelHeight()) - height_;

		if (position_.y >= floorTop)
		{
			position_.y = floorTop;
			velocity_.y = 0.0f;
		}

		return;
	}

	const float ceilingBottom = 0.0f;

	if (position_.y <= ceilingBottom)
	{
		position_.y = ceilingBottom;
		velocity_.y = 0.0f;
	}
}

// タイルマップの'#'を、敵が着地できる足場として扱う。
// 前フレームの位置と現在位置を比べ、敵がタイルの面をまたいだ瞬間に押し戻す。
bool Enemy::ResolveTileVerticalCollision(
	const StageData& currentStage)
{
	const float supportHalfWidth =
		width_ * 0.35f;
	const float enemyCenterX =
		position_.x + width_ * 0.5f;
	const float supportLeft =
		enemyCenterX - supportHalfWidth;
	const float supportRight =
		enemyCenterX + supportHalfWidth;

	bool hitTile = false;
	float resolvedY = position_.y;

	if (gravityDirection_ == GravityDirection::Down)
	{
		if (velocity_.y < 0.0f)
		{
			return false;
		}

		const float previousBottom =
			previousPosition_.y + height_;
		const float currentBottom =
			position_.y + height_;
		float nearestTileTop =
			static_cast<float>(currentStage.GetPixelHeight());

		for (int y = 0; y < currentStage.tileRowCount; ++y)
		{
			for (int x = 0; x < StageData::TileColumns; ++x)
			{
				if (!StageData::IsSolidTile(currentStage.tiles[y][x]))
				{
					continue;
				}

				const int tileLeft =
					x * StageData::TileSize;
				const int tileTop =
					y * StageData::TileSize;
				const int tileRight =
					tileLeft + StageData::TileSize;

				if (IsTileInsideFallHole(tileLeft, tileRight, tileTop, currentStage))
				{
					continue;
				}

				const bool overlapsX =
					supportLeft < static_cast<float>(tileRight) &&
					supportRight > static_cast<float>(tileLeft);
				const bool crossedTileTop =
					previousBottom <= static_cast<float>(tileTop) &&
					currentBottom >= static_cast<float>(tileTop);

				if (overlapsX && crossedTileTop &&
					static_cast<float>(tileTop) < nearestTileTop)
				{
					nearestTileTop = static_cast<float>(tileTop);
					resolvedY = nearestTileTop - height_;
					hitTile = true;
				}
			}
		}
	}
	else
	{
		if (velocity_.y > 0.0f)
		{
			return false;
		}

		const float previousTop =
			previousPosition_.y;
		const float currentTop =
			position_.y;
		float nearestTileBottom = 0.0f;

		for (int y = 0; y < currentStage.tileRowCount; ++y)
		{
			for (int x = 0; x < StageData::TileColumns; ++x)
			{
				if (!StageData::IsSolidTile(currentStage.tiles[y][x]))
				{
					continue;
				}

				const int tileLeft =
					x * StageData::TileSize;
				const int tileTop =
					y * StageData::TileSize;
				const int tileRight =
					tileLeft + StageData::TileSize;
				const int tileBottom =
					tileTop + StageData::TileSize;

				if (IsTileInsideFallHole(tileLeft, tileRight, tileTop, currentStage))
				{
					continue;
				}

				const bool overlapsX =
					supportLeft < static_cast<float>(tileRight) &&
					supportRight > static_cast<float>(tileLeft);
				const bool crossedTileBottom =
					previousTop >= static_cast<float>(tileBottom) &&
					currentTop <= static_cast<float>(tileBottom);

				if (overlapsX && crossedTileBottom &&
					static_cast<float>(tileBottom) > nearestTileBottom)
				{
					nearestTileBottom = static_cast<float>(tileBottom);
					resolvedY = nearestTileBottom;
					hitTile = true;
				}
			}
		}
	}

	if (!hitTile)
	{
		return false;
	}

	position_.y = resolvedY;
	velocity_.y = 0.0f;

	return true;
}

// FallHoleの範囲にある床・天井タイルは、穴として通り抜けられるようにする。
// 中段の浮遊足場は穴の真上にあっても足場として使える。
bool Enemy::IsTileInsideFallHole(
	int tileLeft,
	int tileRight,
	int tileTop,
	const StageData& currentStage) const
{
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];
		if (fallHole.OverlapsTile(tileLeft, tileRight, tileTop,
			tileTop + StageData::TileSize, currentStage.GetPixelHeight(),
			StageData::TileSize * 2))
		{
			return true;
		}
	}

	return false;
}

// 敵の足元判定幅が、どれか1つの落下穴に完全に入っているか調べる。
bool Enemy::IsOverFallHole(const StageData& currentStage) const
{
	const float enemyCenterX =
		position_.x + width_ * 0.5f;
	const float fallCheckHalfWidth =
		width_ * 0.35f;
	const float fallCheckLeft =
		enemyCenterX - fallCheckHalfWidth;
	const float fallCheckRight =
		enemyCenterX + fallCheckHalfWidth;

	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];

		const bool opensGravitySide =
			gravityDirection_ == GravityDirection::Down
			? fallHole.OpensFloor(currentStage.GetPixelHeight())
			: fallHole.OpensCeiling();
		if (opensGravitySide &&
			fallHole.ContainsHorizontal(fallCheckLeft, fallCheckRight))
		{
			return true;
		}
	}

	return false;
}

// 同じX位置にある床穴と天井穴を通り抜けられるようにする。
void Enemy::CheckFallLoop(const StageData& currentStage)
{
	if (currentStage.fallHoleCount <= 0)
	{
		return;
	}

	const float enemyTop = position_.y;
	const float enemyBottom = enemyTop + height_;

	const float centerX = position_.x + width_ * 0.5f;
	const float supportHalfWidth = width_ * 0.35f;
	float pairedExitY = 0.0f;
	if (currentStage.TryGetPairedFallHoleExitY(
		gravityDirection_ == GravityDirection::Down,
		centerX - supportHalfWidth,
		centerX + supportHalfWidth,
		enemyTop,
		enemyBottom,
		height_,
		pairedExitY))
	{
		position_.y = pairedExitY;
		previousPosition_.y = pairedExitY;
		return;
	}

	bool opensFloor = false;
	bool opensCeiling = false;
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& hole = currentStage.fallHoles[i];
		if (centerX >= static_cast<float>(hole.left) &&
			centerX < static_cast<float>(hole.right))
		{
			opensFloor = opensFloor || hole.OpensFloor(currentStage.GetPixelHeight());
			opensCeiling = opensCeiling || hole.OpensCeiling();
		}
	}

	if (opensFloor && enemyTop >= static_cast<float>(
		currentStage.GetPixelHeight() + StageData::FallLoopTravelMargin))
	{
		position_.y = -height_;
		return;
	}

	if (opensCeiling && enemyBottom <=
		-static_cast<float>(StageData::FallLoopTravelMargin))
	{
		position_.y = static_cast<float>(currentStage.GetPixelHeight());
	}
}

// 有効な敵を描画する。
void Enemy::Draw() const
{
	if (!isActive_)
	{
		return;
	}

	const int left = static_cast<int>(position_.x);
	const int top = static_cast<int>(position_.y);
	const int right = static_cast<int>(position_.x + width_);
	const int bottom = static_cast<int>(position_.y + height_);

	int enemyColor = GetColor(255, 80, 80);
	int imageHandle = normalImageHandle_;

	if (type_ == EnemyType::Heavy)
	{
		enemyColor = GetColor(180, 40, 40);
		imageHandle = heavyImageHandle_;
	}
	else if (type_ == EnemyType::Shooter || type_ == EnemyType::UpShooter)
	{
		enemyColor = type_ == EnemyType::UpShooter
			? GetColor(255, 100, 220)
			: GetColor(255, 140, 40);
		imageHandle = shooterImageHandle_;
	}
	else if (type_ == EnemyType::Charger)
	{
		enemyColor = GetColor(50, 190, 255);
		imageHandle = chargerImageHandle_;
	}
	else if (type_ == EnemyType::Mortar)
	{
		enemyColor = GetColor(100, 240, 255);
		imageHandle = mortarImageHandle_;
	}

	// ダメージ直後は点滅させて、攻撃が当たったことを分かりやすくする。
	const bool isDamageBlink =
		damageCooldownFrames_ > 0 &&
		(damageCooldownFrames_ / 4) % 2 == 0;

	if (isDamageBlink)
	{
		return;
	}

	// 画像が設定されている場合は、敵タイプに合ったスプライトを描画する。
	if (imageHandle != -1)
	{
		const bool isFacingLeft = horizontalDirection_ < 0.0f;
		const bool isUpsideDown =
			type_ != EnemyType::Charger &&
			gravityDirection_ == GravityDirection::Up;
		if (type_ == EnemyType::Charger)
		{
			static constexpr int SourceLefts[ChargerAnimationFrameCount] =
				{ 30, 161, 307, 452, 616, 776, 921, 1031, 1176, 1320 };
			static constexpr int SourceWidths[ChargerAnimationFrameCount] =
				{ 100, 114, 113, 113, 94, 79, 79, 114, 114, 188 };
			constexpr int sourceTop = 15;
			constexpr int sourceHeight = 100;
			constexpr float drawScale = 0.8f;
			const int frame = isChargerActivated_ && !isChargerStopped_
				? chargerAnimationFrame_ : 0;
			const int sourceWidth = SourceWidths[frame];
			const int drawWidth = static_cast<int>(sourceWidth * drawScale);
			const int drawHeight = static_cast<int>(sourceHeight * drawScale);
			const int bodyAnchor =
				static_cast<int>((sourceWidth - 45) * drawScale);
			const int bodyCenterX = static_cast<int>(position_.x + width_ * 0.5f);
			const int visualTop = static_cast<int>(position_.y);
			const int visualBottom = visualTop + drawHeight;
			const int visualLeft = isFacingLeft
				? bodyCenterX + bodyAnchor - drawWidth
				: bodyCenterX - bodyAnchor;
			const int visualRight = visualLeft + drawWidth;
			const int sourceLeftX = isFacingLeft ? visualRight : visualLeft;
			const int sourceRightX = isFacingLeft ? visualLeft : visualRight;

			DrawRectModiGraph(
				sourceLeftX, visualTop,
				sourceRightX, visualTop,
				sourceRightX, visualBottom,
				sourceLeftX, visualBottom,
				SourceLefts[frame], sourceTop, sourceWidth, sourceHeight,
				imageHandle, TRUE);
			return;
		}
		if (type_ == EnemyType::Mortar)
		{
			constexpr int sourceLeft = 14;
			constexpr int sourceTop = 13;
			constexpr int sourceWidth = 19;
			constexpr int sourceHeight = 28;
			constexpr int drawWidth = 41;
			constexpr int drawHeight = 60;
			const int visualLeft = static_cast<int>(
				position_.x + (width_ - drawWidth) * 0.5f);
			const int visualTop = static_cast<int>(
				position_.y + (height_ - drawHeight) * 0.5f);
			DrawRectExtendGraph(
				visualLeft, visualTop,
				visualLeft + drawWidth, visualTop + drawHeight,
				sourceLeft, sourceTop,
				sourceWidth, sourceHeight, imageHandle, TRUE);
			return;
		}
		if (type_ == EnemyType::Normal || type_ == EnemyType::Heavy)
		{
			constexpr int frameSize = 64;
			constexpr int framesPerRow = 4;
			constexpr int drawSize = 64;
			const int sourceLeft =
				(walkAnimationFrame_ % framesPerRow) * frameSize;
			const int sourceTop =
				(walkAnimationFrame_ / framesPerRow) * frameSize;
			const int visualLeft = left - (drawSize - (right - left)) / 2;
			const int visualRight = visualLeft + drawSize;
			const int visualTop = isUpsideDown ? top : bottom - drawSize;
			const int visualBottom = isUpsideDown ? top + drawSize : bottom;
			const int sourceLeftX = isFacingLeft ? visualRight : visualLeft;
			const int sourceRightX = isFacingLeft ? visualLeft : visualRight;
			const int sourceTopY = isUpsideDown ? visualBottom : visualTop;
			const int sourceBottomY = isUpsideDown ? visualTop : visualBottom;

			DrawRectModiGraph(
				sourceLeftX, sourceTopY,
				sourceRightX, sourceTopY,
				sourceRightX, sourceBottomY,
				sourceLeftX, sourceBottomY,
				sourceLeft, sourceTop, frameSize, frameSize,
				imageHandle, TRUE);

			return;
		}

		if (type_ == EnemyType::Shooter || type_ == EnemyType::UpShooter)
		{
			constexpr int frameWidth = 64;
			constexpr int sourceLeftPadding = 16;
			constexpr int sourceTop = 20;
			constexpr int sourceWidth = 32;
			constexpr int sourceHeight = 48;
			constexpr int drawHeight = 72;
			const int sourceLeft =
				shooterAnimationFrame_ * frameWidth + sourceLeftPadding;
			const int visualTop = isUpsideDown ? top : bottom - drawHeight;
			const int visualBottom = isUpsideDown ? top + drawHeight : bottom;
			const int sourceLeftX = isFacingLeft ? right : left;
			const int sourceRightX = isFacingLeft ? left : right;
			const int sourceTopY = isUpsideDown ? visualBottom : visualTop;
			const int sourceBottomY = isUpsideDown ? visualTop : visualBottom;

			DrawRectModiGraph(
				sourceLeftX, sourceTopY,
				sourceRightX, sourceTopY,
				sourceRightX, sourceBottomY,
				sourceLeftX, sourceBottomY,
				sourceLeft, sourceTop, sourceWidth, sourceHeight,
				imageHandle, TRUE);

			return;
		}

		const int sourceLeftX = isFacingLeft ? right : left;
		const int sourceRightX = isFacingLeft ? left : right;
		const int sourceTopY = isUpsideDown ? bottom : top;
		const int sourceBottomY = isUpsideDown ? top : bottom;

		DrawModiGraph(
			sourceLeftX, sourceTopY,
			sourceRightX, sourceTopY,
			sourceRightX, sourceBottomY,
			sourceLeftX, sourceBottomY,
			imageHandle, TRUE);

		return;
	}

	DrawBox(
		left,
		top,
		right,
		bottom,
		enemyColor,
		TRUE
	);
}

void Enemy::TakeDamage()
{
	if (!isActive_ || damageCooldownFrames_ > 0)
	{
		return;
	}
	
	--hitPoint_;

	// ダメージを受けた瞬間、進行方向とは逆向きに少し押し戻す。
	if (type_ != EnemyType::Charger && type_ != EnemyType::Mortar)
	{
		knockbackVelocityX_ = -horizontalDirection_ * 6.0f;
	}

	if (hitPoint_ <= 0)
	{
		Deactivate();
		return;
	}

	damageCooldownFrames_ = DamageCooldownDuration;
}

int Enemy::GetHitPoint() const
{
	return hitPoint_;
}

void Enemy::Deactivate()
{
	isActive_ = false;
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
	knockbackVelocityX_ = 0.0f;
}

float Enemy::GetWidthByType(EnemyType type)
{
	if (type == EnemyType::Charger)
	{
		return 80.0f;
	}

	if (type == EnemyType::Mortar)
	{
		return 60.0f;
	}

	if (type == EnemyType::Heavy)
	{
		return 56.0f;
	}

	if (type == EnemyType::Shooter)
	{
		return 48.0f;
	}

	return 48.0f;
}

float Enemy::GetHeightByType(EnemyType type)
{
	if (type == EnemyType::Charger)
	{
		return 80.0f;
	}

	if (type == EnemyType::Mortar)
	{
		return 60.0f;
	}

	if (type == EnemyType::Heavy)
	{
		return 56.0f;
	}

	if (type == EnemyType::Shooter)
	{
		return 48.0f;
	}

	return 48.0f;
}

const Vector2& Enemy::GetPosition() const
{
	return position_;
}

float Enemy::GetWidth() const
{
	return width_;
}

float Enemy::GetHeight() const
{
	return height_;
}

bool Enemy::IsActive() const
{
	return isActive_;
}

EnemyType Enemy::GetType() const
{
	return type_;
}

