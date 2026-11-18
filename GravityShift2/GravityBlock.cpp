#include "GravityBlock.h"

#include <cmath>

#include "GameConfig.h"
#include "StageData.h"
#include "DxLib.h"

// ブロックを初期位置へ配置する。
void GravityBlock::Initialize(
	const Vector2& position,
	GravityDirection gravityDirection,
	GravityBlockMotion motion)
{
	position_ = position;
	previousPosition_ = position_;
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
	gravityDirection_ = gravityDirection;
	motion_ = motion;
	motionMinimum_ = 0.0f;
	motionMaximum_ = 0.0f;
	automaticWaitTimer_ = automaticWaitFrames_;
	automaticMoveDirection_ = -1;
	motionBoundsInitialized_ = false;
	landedThisFrame_ = false;
	isActive_ = true;
}

// ブロックを未使用状態にする。
void GravityBlock::Deactivate()
{
	isActive_ = false;
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
	motionBoundsInitialized_ = false;
	automaticWaitTimer_ = 0;
	landedThisFrame_ = false;
}

// ブロックの1フレーム分の動きを更新する。
void GravityBlock::Update(
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	if (!isActive_)
	{
		return;
	}

	landedThisFrame_ = false;
	previousPosition_ = position_;

	if (motion_ != GravityBlockMotion::Gravity)
	{
		UpdateAutomaticMotion(currentStage);
		return;
	}

	const bool wasFalling = IsFalling();

	ApplyGravity(gravitySystem);
	ResolveVerticalCollision(currentStage);
	CheckFallLoop(currentStage);

	landedThisFrame_ = wasFalling && !IsFalling();
}

// 重力反転時にブロックの重力方向を切り替える。
void GravityBlock::OnGravityReversed()
{
	if (!isActive_ || motion_ != GravityBlockMotion::Gravity)
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

// 配置地点から見て最寄りにある地形の内側を、自動往復の両端にする。
void GravityBlock::InitializeMotionBounds(const StageData& currentStage)
{
	if (motion_ == GravityBlockMotion::Vertical)
	{
		motionMinimum_ = 0.0f;
		motionMaximum_ = static_cast<float>(currentStage.GetPixelHeight()) - height_;

		for (int y = 0; y < currentStage.tileRowCount; ++y)
		{
			for (int x = 0; x < StageData::TileColumns; ++x)
			{
				if (!StageData::IsSolidTile(currentStage.tiles[y][x]))
				{
					continue;
				}

				const float tileLeft = static_cast<float>(x * StageData::TileSize);
				const float tileRight = tileLeft + StageData::TileSize;
				if (position_.x >= tileRight || position_.x + width_ <= tileLeft)
				{
					continue;
				}

				const float tileTop = static_cast<float>(y * StageData::TileSize);
				const float tileBottom = tileTop + StageData::TileSize;
				if (tileBottom <= position_.y + 0.1f && tileBottom > motionMinimum_)
				{
					motionMinimum_ = tileBottom;
				}
				if (tileTop >= position_.y + height_ - 0.1f &&
					tileTop - height_ < motionMaximum_)
				{
					motionMaximum_ = tileTop - height_;
				}
			}
		}

		if (motionMaximum_ < motionMinimum_)
		{
			motionMaximum_ = motionMinimum_;
		}

		const bool startAtMinimum =
			std::fabs(position_.y - motionMinimum_) <
			std::fabs(position_.y - motionMaximum_);
		position_.y = startAtMinimum ? motionMinimum_ : motionMaximum_;
		automaticMoveDirection_ = startAtMinimum ? 1 : -1;
	}
	else
	{
		motionMinimum_ = 0.0f;
		motionMaximum_ = static_cast<float>(StageData::StagePixelWidth) - width_;

		for (int y = 0; y < currentStage.tileRowCount; ++y)
		{
			for (int x = 0; x < StageData::TileColumns; ++x)
			{
				if (!StageData::IsSolidTile(currentStage.tiles[y][x]))
				{
					continue;
				}

				const float tileTop = static_cast<float>(y * StageData::TileSize);
				const float tileBottom = tileTop + StageData::TileSize;
				if (position_.y >= tileBottom || position_.y + height_ <= tileTop)
				{
					continue;
				}

				const float tileLeft = static_cast<float>(x * StageData::TileSize);
				const float tileRight = tileLeft + StageData::TileSize;
				if (tileRight <= position_.x + 0.1f && tileRight > motionMinimum_)
				{
					motionMinimum_ = tileRight;
				}
				if (tileLeft >= position_.x + width_ - 0.1f &&
					tileLeft - width_ < motionMaximum_)
				{
					motionMaximum_ = tileLeft - width_;
				}
			}
		}

		if (motionMaximum_ < motionMinimum_)
		{
			motionMaximum_ = motionMinimum_;
		}

		const bool startAtMinimum =
			std::fabs(position_.x - motionMinimum_) <
			std::fabs(position_.x - motionMaximum_);
		position_.x = startAtMinimum ? motionMinimum_ : motionMaximum_;
		automaticMoveDirection_ = startAtMinimum ? 1 : -1;
	}

	previousPosition_ = position_;
	velocity_ = {};
	automaticWaitTimer_ = automaticWaitFrames_;
	motionBoundsInitialized_ = true;
}

// 待機時間が終わったら反対側の地形まで一定速度で移動する。
void GravityBlock::UpdateAutomaticMotion(const StageData& currentStage)
{
	if (!motionBoundsInitialized_)
	{
		InitializeMotionBounds(currentStage);
	}

	velocity_ = {};
	if (automaticWaitTimer_ > 0 || motionMinimum_ == motionMaximum_)
	{
		if (automaticWaitTimer_ > 0)
		{
			--automaticWaitTimer_;
		}
		return;
	}

	const float movement = automaticMoveSpeed_ * automaticMoveDirection_;
	float& coordinate = motion_ == GravityBlockMotion::Vertical
		? position_.y : position_.x;
	float& speed = motion_ == GravityBlockMotion::Vertical
		? velocity_.y : velocity_.x;
	coordinate += movement;
	speed = movement;

	const bool reachedMinimum =
		automaticMoveDirection_ < 0 && coordinate <= motionMinimum_;
	const bool reachedMaximum =
		automaticMoveDirection_ > 0 && coordinate >= motionMaximum_;
	if (!reachedMinimum && !reachedMaximum)
	{
		return;
	}

	coordinate = reachedMinimum ? motionMinimum_ : motionMaximum_;
	speed = 0.0f;
	automaticMoveDirection_ *= -1;
	automaticWaitTimer_ = automaticWaitFrames_;
}

// 現在の重力方向へ落下速度を加える。
void GravityBlock::ApplyGravity(const GravitySystem& gravitySystem)
{
	float acceleration = gravitySystem.GetAcceleration();

	if (acceleration < 0.0f)
	{
		acceleration *= -1.0f;
	}

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

// 床または天井にぶつかったら、その位置で止める。
void GravityBlock::ResolveVerticalCollision(
	const StageData& currentStage)
{
	// 浮いているタイル足場に当たった場合は、そこで着地させる。
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

// タイルマップの'#'を、重力ブロックが着地できる足場として扱う。
// ブロックがタイルの上面または下面をまたいだ瞬間に、タイルの外側へ押し戻す。
bool GravityBlock::ResolveTileVerticalCollision(
	const StageData& currentStage)
{
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
					position_.x < static_cast<float>(tileRight) &&
					position_.x + width_ > static_cast<float>(tileLeft);
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
					position_.x < static_cast<float>(tileRight) &&
					position_.x + width_ > static_cast<float>(tileLeft);
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
bool GravityBlock::IsTileInsideFallHole(
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

// ブロックの中心寄りの幅が、どれか1つの落下穴に完全に入っているか調べる。
bool GravityBlock::IsOverFallHole(const StageData& currentStage) const
{
	const float blockCenterX =
		position_.x + width_ * 0.5f;
	const float fallCheckHalfWidth =
		width_ * 0.35f;
	const float fallCheckLeft =
		blockCenterX - fallCheckHalfWidth;
	const float fallCheckRight =
		blockCenterX + fallCheckHalfWidth;

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
void GravityBlock::CheckFallLoop(const StageData& currentStage)
{
	if (currentStage.fallHoleCount <= 0)
	{
		return;
	}

	const float blockTop = position_.y;
	const float blockBottom = blockTop + height_;

	const float centerX = position_.x + width_ * 0.5f;
	const float supportHalfWidth = width_ * 0.35f;
	float pairedExitY = 0.0f;
	if (currentStage.TryGetPairedFallHoleExitY(
		gravityDirection_ == GravityDirection::Down,
		centerX - supportHalfWidth,
		centerX + supportHalfWidth,
		blockTop,
		blockBottom,
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

	if (opensFloor && blockTop >= static_cast<float>(
		currentStage.GetPixelHeight() + StageData::FallLoopTravelMargin))
	{
		position_.y = -height_;
		return;
	}

	if (opensCeiling && blockBottom <=
		-static_cast<float>(StageData::FallLoopTravelMargin))
	{
		position_.y = static_cast<float>(currentStage.GetPixelHeight());
	}
}

// ブロックを描画する。
void GravityBlock::Draw() const
{
	if (!isActive_)
	{
		return;
	}

	const int left = static_cast<int>(position_.x);
	const int top = static_cast<int>(position_.y);
	const int right = static_cast<int>(position_.x + width_);
	const int bottom = static_cast<int>(position_.y + height_);

	// 画像が読み込まれている場合はスプライトを使う。
	if (imageHandle_ != -1)
	{
		DrawExtendGraph(
			left,
			top,
			right,
			bottom,
			imageHandle_,
			TRUE
		);
		return;
	}

	// 画像がない場合は、今まで通り仮の矩形を描画する。
	DrawBox(
		left,
		top,
		right,
		bottom,
		GetColor(180, 180, 220),
		TRUE
	);

	DrawBox(
		left,
		top,
		right,
		bottom,
		GetColor(255, 255, 255),
		FALSE
	);
}

void GravityBlock::SetImageHandle(int imageHandle)
{
	imageHandle_ = imageHandle;
}

const Vector2& GravityBlock::GetPosition() const
{
	return position_;
}

float GravityBlock::GetWidth() const
{
	return width_;
}

float GravityBlock::GetHeight() const
{
	return height_;
}

bool GravityBlock::IsFalling() const
{
	if (!isActive_ || motion_ != GravityBlockMotion::Gravity)
	{
		return false;
	}

	if (gravityDirection_ == GravityDirection::Down)
	{
		return velocity_.y > 0.1f;
	}

	return velocity_.y < -0.1f;
}

bool GravityBlock::IsDamaging() const
{
	return isActive_ &&
		(motion_ != GravityBlockMotion::Gravity || IsFalling());
}

bool GravityBlock::WasLandedThisFrame() const
{
	return landedThisFrame_;
}

GravityDirection GravityBlock::GetGravityDirection() const
{
	return gravityDirection_;
}

bool GravityBlock::IsActive() const
{
	return isActive_;
}

