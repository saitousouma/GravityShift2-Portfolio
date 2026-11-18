#include "Player.h"
#include "GravityBlock.h"
#include "Bullet.h"
#include "InputManager.h"
#include "GameConfig.h"
#include "GravitySystem.h"
#include "StageData.h"
#include "DxLib.h"

#include <algorithm>
#include <string>

namespace
{
	constexpr int SpriteStride = 96;
	constexpr int SpriteSourceX = 16;
	constexpr int SpriteSourceY = 24;
	constexpr int SpriteWidth = 68;
	constexpr int SpriteHeight = 72;
	constexpr int HurtFrameCount = 4;
	constexpr int HurtFrameTicks = 4;
	constexpr int DeathFrameCount = 9;
	constexpr int DeathFrameTicks = 5;
	constexpr int ShootFrameCount = 3;
	constexpr int JumpFrameCount = 6;
	constexpr int JumpRiseFrameCount = 4;
	constexpr float BulletSpawnInset = 6.0f;

	struct SpriteClip
	{
		int handle;
		int frameCount;
		int frameTicks;
		bool loop;
	};

	int LoadPlayerSprite(const wchar_t* fileName, int frameCount)
	{
		const std::wstring file = fileName;
		int handle = LoadGraph(
			(L"Assets/Images/Player/" + file).c_str());
		if (handle == -1)
		{
			handle = LoadGraph(
				(L"../../Assets/Images/Player/" + file).c_str());
		}
		int width = 0;
		int height = 0;
		if (handle != -1 &&
			(GetGraphSize(handle, &width, &height) != 0 ||
			width < (frameCount - 1) * SpriteStride + SpriteSourceX + SpriteWidth ||
			height < SpriteSourceY + SpriteHeight))
		{
			DeleteGraph(handle);
			handle = -1;
		}
		return handle;
	}
}

void Player::LoadSprites()
{
	ReleaseSprites();
	idleSprite_ = LoadPlayerSprite(L"player-idle.png", 10);
	runSprite_ = LoadPlayerSprite(L"player-run.png", 8);
	dashSprite_ = LoadPlayerSprite(L"player-dash.png", 6);
	jumpSprite_ = LoadPlayerSprite(L"player-jump.png", JumpFrameCount);
	jumpRiseSprite_ = LoadPlayerSprite(
		L"player-jump-rise.png", JumpRiseFrameCount);
	shootSprite_ = LoadPlayerSprite(L"player-shoot.png", ShootFrameCount);
	hurtSprite_ = LoadPlayerSprite(L"player-hurt.png", HurtFrameCount);
	deathSprite_ = LoadPlayerSprite(L"player-death.png", DeathFrameCount);
}

void Player::ReleaseSprites()
{
	int* handles[] = { &idleSprite_, &runSprite_, &dashSprite_,
		&jumpSprite_, &jumpRiseSprite_, &shootSprite_,
		&hurtSprite_, &deathSprite_ };
	for (int* handle : handles)
	{
		if (*handle != -1)
		{
			DeleteGraph(*handle);
			*handle = -1;
		}
	}
}

// プレイヤーを初期状態に戻す。
// 将来、ミス後のリスタートやチェックポイント復帰にも使える形にしておく。
void Player::Initialize()
{
	hitPoint_ = MaxHitPoint;
	invincibleTimer_ = 0;
	ResetToStartPosition();
}

void Player::Initialize(const Vector2& startPosition)
{
	hitPoint_ = MaxHitPoint;
	invincibleTimer_ = 0;
	ResetToStartPosition(startPosition);
}

// プレイヤーの1フレーム分の更新。
// 処理順を上から読むだけで、移動、ジャンプ、重力、衝突、状態更新の流れが分かるようにしている。
void Player::Update(
	const InputManager& inputManager,
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	wasJumpedThisFrame_ = false;
	wasDashStartedThisFrame_ = false;
	previousPosition_ = position_;

	if (invincibleTimer_ > 0)
	{
		--invincibleTimer_;
	}

	UpdateDash(inputManager);
	const bool keepsDashHeightThisFrame = isDashing_;
	UpdateHorizontalMovement(inputManager);
	ResolveHorizontalCollision(currentStage);
	if (keepsDashHeightThisFrame)
	{
		position_.y = dashHeight_;
		velocity_.y = 0.0f;
		isGrounded_ = false;
	}
	else
	{
		UpdateJump(inputManager, gravitySystem);
		ApplyGravity(gravitySystem);
		ResolveVerticalCollision(gravitySystem, currentStage);
	}
	isGravityUp_ = gravitySystem.GetDirection() == GravityDirection::Up;
	if (hurtAnimationTick_ >= 0 &&
		hurtAnimationTick_ < HurtFrameCount * HurtFrameTicks)
	{
		++hurtAnimationTick_;
	}
	if (hurtAnimationTick_ >= HurtFrameCount * HurtFrameTicks)
	{
		hurtAnimationTick_ = -1;
	}
	UpdateState();
	if (animationTick_ < 1000000)
	{
		++animationTick_;
	}
}

void Player::SetState(PlayerState state)
{
	if (state_ != state)
	{
		state_ = state;
		animationTick_ = 0;
	}
}

// 現在の移動状態から、アニメーションやデバッグ表示用の状態を決める。
void Player::UpdateState()
{
	if (shootStateTimer_ > 0)
	{
		--shootStateTimer_;
	}

	// レーザー中は通常射撃や移動の表示より優先する。
	if (isLaserActive_)
	{
		SetState(PlayerState::Laser);
		return;
	}

	if (shootStateTimer_ > 0)
	{
		SetState(PlayerState::Shoot);
		return;
	}

	if (isDashing_ && velocity_.x != 0.0f)
	{
		SetState(PlayerState::Dash);
		return;
	}

	if (!isGrounded_)
	{
		SetState(PlayerState::Jump);
		return;
	}

	if (velocity_.x != 0.0f)
	{
		SetState(PlayerState::Run);
		return;
	}

	SetState(PlayerState::Idle);
}

// ShiftまたはコントローラーのXを押した瞬間に、
// 入力方向または現在向いている方向へ固定距離ダッシュする。
void Player::UpdateDash(const InputManager& inputManager)
{
	if (dashCooldownFrames_ > 0)
	{
		--dashCooldownFrames_;
	}

	if (isDashing_)
	{
		if (dashDistanceTravelled_ >= DashDistance)
		{
			EndDash();
		}
		return;
	}

	if (dashCooldownFrames_ > 0)
	{
		return;
	}

	// キーボードとコントローラーの割り当てはInputManager側でまとめる。
	const bool isDashTriggered = inputManager.IsDashTriggered();

	if (!isDashTriggered)
	{
		return;
	}

	int direction = static_cast<int>(facingDirection_);
	const bool isLeftPressed = inputManager.IsPressed(KEY_INPUT_LEFT);
	const bool isRightPressed = inputManager.IsPressed(KEY_INPUT_RIGHT);
	if (isLeftPressed != isRightPressed)
	{
		direction = isLeftPressed ? -1 : 1;
	}

	StartDash(direction);
}

void Player::StartDash(int direction)
{
	dashDirection_ = direction < 0 ? -1 : 1;
	facingDirection_ = dashDirection_ < 0
		? FacingDirection::Left
		: FacingDirection::Right;
	dashDistanceTravelled_ = 0.0f;
	dashHeight_ = position_.y;
	velocity_.y = 0.0f;
	knockbackVelocityX_ = 0.0f;
	isGrounded_ = false;
	isDashing_ = true;
	wasDashStartedThisFrame_ = true;
}

void Player::EndDash()
{
	const bool didEndDash = isDashing_;
	isDashing_ = false;
	dashDistanceTravelled_ = 0.0f;
	velocity_.x = 0.0f;

	if (didEndDash)
	{
		dashCooldownFrames_ = DashCooldownFrames;
	}
}

// 左右入力を横速度へ変換し、ステージ外へ出ないように補正する。
void Player::UpdateHorizontalMovement(
	const InputManager& inputManager)
{
	velocity_.x = 0.0f;
	if (isDashing_)
	{
		const float remainingDistance =
			DashDistance - dashDistanceTravelled_;
		const float dashStep = (std::min)(dashSpeed_, remainingDistance);
		velocity_.x = dashStep * static_cast<float>(dashDirection_);
		position_.x += velocity_.x;
		dashDistanceTravelled_ += dashStep;

		const float rightLimit =
			static_cast<float>(StageData::StagePixelWidth) - width_;
		if (position_.x <= 0.0f)
		{
			position_.x = 0.0f;
			EndDash();
		}
		else if (position_.x >= rightLimit)
		{
			position_.x = rightLimit;
			EndDash();
		}
		return;
	}

	if (inputManager.IsPressed(KEY_INPUT_LEFT))
	{
		velocity_.x -= moveSpeed_;
		facingDirection_ = FacingDirection::Left;
	}

	if (inputManager.IsPressed(KEY_INPUT_RIGHT))
	{
		velocity_.x += moveSpeed_;
		facingDirection_ = FacingDirection::Right;
	}

	position_.x += velocity_.x + knockbackVelocityX_;

	// ノックバックは毎フレーム少しずつ弱め、自然に止まるようにする。
	knockbackVelocityX_ *= KnockbackDecay;

	if (knockbackVelocityX_ > -0.1f && knockbackVelocityX_ < 0.1f)
	{
		knockbackVelocityX_ = 0.0f;
	}

	if (position_.x < 0.0f)
	{
		position_.x = 0.0f;
	}

	const float rightLimit =
		static_cast<float>(StageData::StagePixelWidth) - width_;

	if (position_.x > rightLimit)
	{
		position_.x = rightLimit;
	}
}

void Player::ResolveHorizontalCollision(const StageData& currentStage)
{
	const float playerLeft = position_.x;
	const float playerTop = position_.y;
	const float playerRight = playerLeft + width_;
	const float playerBottom = playerTop + height_;

	for (int y = 0; y < currentStage.tileRowCount; ++y)
	{
		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			if (!StageData::IsSolidTile(currentStage.tiles[y][x]))
			{
				continue;
			}

			const float tileLeft =
				static_cast<float>(x * StageData::TileSize);
			const float tileTop =
				static_cast<float>(y * StageData::TileSize);
			const float tileRight =
				tileLeft + static_cast<float>(StageData::TileSize);
			const float tileBottom =
				tileTop + static_cast<float>(StageData::TileSize);

			const bool overlapsY =
				playerTop + 2.0f < tileBottom &&
				playerBottom - 2.0f > tileTop;

			if (!overlapsY)
			{
				continue;
			}

			const bool overlapsX =
				playerLeft < tileRight &&
				playerRight > tileLeft;

			if (!overlapsX)
			{
				continue;
			}

			if (velocity_.x > 0.0f || knockbackVelocityX_ > 0.0f)
			{
				position_.x = tileLeft - width_;
				velocity_.x = 0.0f;
				knockbackVelocityX_ = 0.0f;
				EndDash();
				return;
			}

			if (velocity_.x < 0.0f || knockbackVelocityX_ < 0.0f)
			{
				position_.x = tileRight;
				velocity_.x = 0.0f;
				knockbackVelocityX_ = 0.0f;
				EndDash();
				return;
			}

			const float pushLeftDistance =
				playerRight - tileLeft;
			const float pushRightDistance =
				tileRight - playerLeft;

			// ブロックなど外部の押し戻しで速度0のまま壁に入った場合は、
			// 食い込みが浅い方向へ逃がして、壁の中に残らないようにする。
			if (pushLeftDistance < pushRightDistance)
			{
				position_.x = tileLeft - width_;
			}
			else
			{
				position_.x = tileRight;
			}

			velocity_.x = 0.0f;
			knockbackVelocityX_ = 0.0f;
			EndDash();
			return;
		}
	}
}

void Player::ResolveStageHorizontalCollision(const StageData& currentStage)
{
	ResolveHorizontalCollision(currentStage);
}

// 接地中にスペースキーまたはコントローラーのAを押した瞬間だけジャンプする。
// 重力が上向きのときは、ジャンプ方向も反転する。
void Player::UpdateJump(
	const InputManager& inputManager,
	const GravitySystem& gravitySystem)
{
	if (!isGrounded_)
	{
		return;
	}

	if (!inputManager.IsJumpTriggered())
	{
		return;
	}

	const int gravitySign =
		static_cast<int>(gravitySystem.GetDirection());

	velocity_.y = -jumpSpeed_ * gravitySign;
	isGrounded_ = false;
	wasJumpedThisFrame_ = true;
}

// 重力加速度を縦速度へ加え、その速度をY座標へ反映する。
void Player::ApplyGravity(
	const GravitySystem& gravitySystem)
{
	float acceleration = gravitySystem.GetAcceleration();
	const bool isFalling =
		(gravitySystem.GetDirection() == GravityDirection::Down &&
			velocity_.y >= 0.0f) ||
		(gravitySystem.GetDirection() == GravityDirection::Up &&
			velocity_.y <= 0.0f);
	if (isFalling)
	{
		acceleration *= FallAccelerationScale;
	}
	velocity_.y += acceleration;

	// 落下速度が上がり続けないように、重力方向ごとに最大速度を制限する。
	if (gravitySystem.GetDirection() == GravityDirection::Down &&
		velocity_.y > maxFallSpeed_)
	{
		velocity_.y = maxFallSpeed_;
	}
	else if (gravitySystem.GetDirection() == GravityDirection::Up &&
		velocity_.y < -maxFallSpeed_)
	{
		velocity_.y = -maxFallSpeed_;
	}

	position_.y += velocity_.y;
}

// 現在の重力方向に応じて、床または天井との接地を解決する。
// isGrounded_は「現在の重力方向側の面に接地しているか」を表す。
void Player::ResolveVerticalCollision(
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	// 先にタイル足場との接触を見る。
	// 浮いている足場に触れた場合はここで接地が完了する。
	if (ResolveTileVerticalCollision(gravitySystem, currentStage))
	{
		return;
	}

	const float playerCenterX =
		position_.x + width_ * 0.5f;
	const float fallCheckHalfWidth =
		fallHoleCheckWidth_ * 0.5f;
	const float fallCheckLeft =
		playerCenterX - fallCheckHalfWidth;
	const float fallCheckRight =
		playerCenterX + fallCheckHalfWidth;

	bool isOverFallHole = false;

	// 足元の接地幅が、どれか1つの穴の中に完全に入っているか調べる。
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];
		const bool opensGravitySide =
			gravitySystem.GetDirection() == GravityDirection::Down
			? fallHole.OpensFloor(currentStage.GetPixelHeight())
			: fallHole.OpensCeiling();

		if (opensGravitySide &&
			fallHole.ContainsHorizontal(fallCheckLeft, fallCheckRight))
		{
			isOverFallHole = true;
			break;
		}
	}

	// 穴の上では床にも天井にも接地させず、そのまま落下させる。
	if (isOverFallHole)
	{
		isGrounded_ = false;
		return;
	}

	const float floorTop =
		static_cast<float>(currentStage.GetPixelHeight()) - height_;

	if (gravitySystem.GetDirection() == GravityDirection::Down)
	{
		if (position_.y >= floorTop)
		{
			position_.y = floorTop;
			velocity_.y = 0.0f;
			isGrounded_ = true;
		}
		else
		{
			isGrounded_ = false;
		}

		return;
	}

	const float ceilingBottom = 0.0f;

	if (position_.y <= ceilingBottom)
	{
		position_.y = ceilingBottom;
		velocity_.y = 0.0f;
		isGrounded_ = true;
	}
	else
	{
		isGrounded_ = false;
	}
}

// 当たり判定はそのままに、状態に応じたコマをプレイヤー中心へ描く。
void Player::Draw() const
{
	// ダメージ直後の無敵時間中は、プレイヤーを点滅させる。
	if (!IsDead() && hurtAnimationTick_ < 0 &&
		invincibleTimer_ > 0 && (invincibleTimer_ / 5) % 2 == 0)
	{
		return;
	}

	SpriteClip clip{ idleSprite_, 10, 8, true };
	int tick = animationTick_;
	if (IsDead())
	{
		clip = { deathSprite_, DeathFrameCount, DeathFrameTicks, false };
		tick = deathAnimationTick_;
	}
	else if (hurtAnimationTick_ >= 0)
	{
		clip = { hurtSprite_, HurtFrameCount, HurtFrameTicks, false };
		tick = hurtAnimationTick_;
	}
	else
	{
		switch (state_)
		{
		case PlayerState::Run:
			clip = { runSprite_, 8, 5, true };
			break;
		case PlayerState::Dash:
			clip = { dashSprite_, 6, 4, true };
			break;
		case PlayerState::Jump:
		{
			const bool isRising = isGravityUp_
				? velocity_.y > 0.0f
				: velocity_.y < 0.0f;
			clip = isRising
				? SpriteClip{ jumpRiseSprite_, JumpRiseFrameCount, 4, false }
				: SpriteClip{ jumpSprite_, JumpFrameCount, 4, false };
			break;
		}
		case PlayerState::Shoot:
			clip = { shootSprite_, ShootFrameCount, 3, false };
			break;
		case PlayerState::Laser:
			clip = { shootSprite_, ShootFrameCount, 5, false };
			break;
		default:
			break;
		}
	}

	if (clip.handle == -1 && idleSprite_ != -1)
	{
		clip = { idleSprite_, 10, 8, true };
		tick = animationTick_;
	}

	if (clip.handle != -1)
	{
		const int frame = clip.loop
			? (tick / clip.frameTicks) % clip.frameCount
			: (std::min)(tick / clip.frameTicks, clip.frameCount - 1);
		const int centerX = static_cast<int>(position_.x + width_ * 0.5f);
		const int centerY = static_cast<int>(position_.y +
			(isGravityUp_ ? SpriteHeight * 0.5f
				: height_ - SpriteHeight * 0.5f));
		DrawRectRotaGraph(centerX, centerY,
			frame * SpriteStride + SpriteSourceX, SpriteSourceY,
			SpriteWidth, SpriteHeight, 1.0, 0.0, clip.handle, TRUE,
			facingDirection_ == FacingDirection::Left, isGravityUp_);
		return;
	}

	// 画像が読み込めない環境では、これまでどおりの仮表示へ戻す。
	const int left = static_cast<int>(position_.x);
	const int top = static_cast<int>(position_.y);
	const int right = static_cast<int>(position_.x + width_);
	const int bottom = static_cast<int>(position_.y + height_);

	DrawBox(
		left,
		top,
		right,
		bottom,
		GetColor(80, 200, 255),
		TRUE
	);
}

// 重力反転直後に、前の床・天井への接地状態を解除する。
// 反転前の落下速度もリセットし、反対方向へ素直に落ち始めるようにする。
void Player::OnGravityReversed()
{
	isGrounded_ = false;
	velocity_.y = 0.0f;
	isGravityUp_ = !isGravityUp_;
}

// 射撃した瞬間に呼ばれ、短時間だけShoot状態を表示する。
void Player::OnShoot()
{
	state_ = PlayerState::Shoot;
	animationTick_ = 0;
	shootStateTimer_ = ShootStateDuration;
}

// Laser状態を表示する。
void Player::OnLaserFire()
{
	SetState(PlayerState::Laser);
}

void Player::AdvanceDeathAnimation()
{
	if (IsDead() && deathAnimationTick_ < DeathFrameCount * DeathFrameTicks)
	{
		++deathAnimationTick_;
	}
}

void Player::SetLaserActive(bool isActive)
{
	isLaserActive_ = isActive;
}

// プレイヤーをステージ開始位置に戻す。
// ステージクリア後やミス後の復帰で使う。
void Player::ResetToStartPosition()
{
	ResetToStartPosition({ 200.0f, 500.0f });
}

void Player::ResetToStartPosition(const Vector2& startPosition)
{
	position_ = startPosition;
	previousPosition_ = position_;

	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
	knockbackVelocityX_ = 0.0f;

	isGrounded_ = false;
	isDashing_ = false;
	dashDistanceTravelled_ = 0.0f;
	dashHeight_ = position_.y;
	dashDirection_ = 1;
	dashCooldownFrames_ = 0;
	isLaserActive_ = false;
	isGravityUp_ = false;
	wasJumpedThisFrame_ = false;
	wasDashStartedThisFrame_ = false;
	shootStateTimer_ = 0;
	animationTick_ = 0;
	hurtAnimationTick_ = -1;
	deathAnimationTick_ = 0;

	state_ = PlayerState::Idle;
	facingDirection_ = FacingDirection::Right;
}

// 接触ダメージを1点与え、無敵時間とノックバックを開始する。
bool Player::TakeDamage(float damageSourceCenterX)
{
	if (hitPoint_ <= 0 || invincibleTimer_ > 0)
	{
		return false;
	}

	EndDash();
	--hitPoint_;
	invincibleTimer_ = InvincibleFrames;
	hurtAnimationTick_ = 0;
	if (hitPoint_ <= 0)
	{
		deathAnimationTick_ = 0;
	}
	ApplyDamageKnockback(damageSourceCenterX);

	return true;
}

bool Player::Heal(int amount)
{
	if (amount <= 0 || hitPoint_ <= 0 || hitPoint_ >= MaxHitPoint)
	{
		return false;
	}

	hitPoint_ += amount;
	if (hitPoint_ > MaxHitPoint)
	{
		hitPoint_ = MaxHitPoint;
	}
	return true;
}

void Player::ApplyDamageKnockback(float damageSourceCenterX)
{
	const float playerCenterX =
		position_.x + width_ * 0.5f;

	// ダメージ元が左にあるなら右へ、右にあるなら左へ押し返す。
	if (damageSourceCenterX < playerCenterX)
	{
		knockbackVelocityX_ = DamageKnockbackSpeed;
	}
	else
	{
		knockbackVelocityX_ = -DamageKnockbackSpeed;
	}
}

// タイルマップの'#'を、床・天井・浮遊足場として扱う。
// 前フレームの位置と現在位置を比べ、プレイヤーがタイルの面をまたいだ瞬間に押し戻す。
bool Player::ResolveTileVerticalCollision(
	const GravitySystem& gravitySystem,
	const StageData& currentStage)
{
	const float supportHalfWidth =
		fallHoleCheckWidth_ * 0.5f;
	const float playerCenterX =
		position_.x + width_ * 0.5f;
	const float supportLeft =
		playerCenterX - supportHalfWidth;
	const float supportRight =
		playerCenterX + supportHalfWidth;

	const float previousTop =
		previousPosition_.y;
	const float previousBottom =
		previousPosition_.y + height_;
	const float currentTop =
		position_.y;
	const float currentBottom =
		position_.y + height_;

	bool hitTile = false;
	bool hitGravitySide = false;
	float resolvedY = position_.y;

	if (velocity_.y > 0.0f)
	{
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
					hitGravitySide =
						gravitySystem.GetDirection() == GravityDirection::Down;
				}
			}
		}
	}
	else if (velocity_.y < 0.0f)
	{
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
					hitGravitySide =
						gravitySystem.GetDirection() == GravityDirection::Up;
				}
			}
		}
	}

	if (!hitTile)
	{
		isGrounded_ = false;
		return false;
	}

	position_.y = resolvedY;
	velocity_.y = 0.0f;
	isGrounded_ = hitGravitySide;

	return true;
}

// FallHoleの範囲にある床・天井タイルは、穴として通り抜けられるようにする。
// 中段の浮遊足場は穴の真上にあっても足場として使える。
bool Player::IsTileInsideFallHole(
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

// 弾の発射開始位置を返す。
// プレイヤーの向きに応じて、右端または左端から発射する。
Vector2 Player::GetShootPosition() const
{
	Vector2 shootPosition{};

	if (facingDirection_ == FacingDirection::Right)
	{
		shootPosition.x = position_.x + width_ - BulletSpawnInset;
	}
	else
	{
		shootPosition.x = position_.x - Bullet::HorizontalWidth +
			BulletSpawnInset;
	}

	shootPosition.y =
		position_.y + height_ * 0.5f - Bullet::HorizontalHeight * 0.5f;

	return shootPosition;
}

// プレイヤーの見た目上の頭側から、縦弾の左上座標を返す。
Vector2 Player::GetHeadShootPosition() const
{
	Vector2 shootPosition{};
	shootPosition.x =
		position_.x + width_ * 0.5f - Bullet::VerticalWidth * 0.5f;
	shootPosition.y = isGravityUp_
		? position_.y + height_ - BulletSpawnInset
		: position_.y - Bullet::VerticalHeight + BulletSpawnInset;
	return shootPosition;
}

// プレイヤーの見た目上の足元側から、縦弾の左上座標を返す。
Vector2 Player::GetFootShootPosition() const
{
	Vector2 shootPosition{};
	shootPosition.x =
		position_.x + width_ * 0.5f - Bullet::VerticalWidth * 0.5f;
	shootPosition.y = isGravityUp_
		? position_.y - Bullet::VerticalHeight + BulletSpawnInset
		: position_.y + height_ - BulletSpawnInset;
	return shootPosition;
}

Vector2 Player::GetLaserStartPosition() const
{
	Vector2 laserPosition{};

	if (facingDirection_ == FacingDirection::Right)
	{
		laserPosition.x = position_.x + width_;
	}
	else
	{
		laserPosition.x = position_.x;
	}

	laserPosition.y =
		position_.y + height_ * 0.5f;

	return laserPosition;
}

const Vector2& Player::GetPosition() const
{
	return position_;
}

void Player::SetPositionY(float y)
{
	position_.y = y;
	previousPosition_.y = y;
	if (isDashing_)
	{
		dashHeight_ = y;
	}
}

// 重力ブロックを横からすり抜けないように、左右方向へ押し戻す。
// 前フレームの位置を使い、左から入ったのか右から入ったのかを判定する。
bool Player::ResolveBlockHorizontalCollision(const GravityBlock& gravityBlock)
{
	if (!gravityBlock.IsActive())
	{
		return false;
	}

	const float playerLeft = position_.x;
	const float playerTop = position_.y;
	const float playerRight = playerLeft + width_;
	const float playerBottom = playerTop + height_;

	const float previousPlayerLeft = previousPosition_.x;
	const float previousPlayerRight = previousPlayerLeft + width_;

	const float blockLeft = gravityBlock.GetPosition().x;
	const float blockTop = gravityBlock.GetPosition().y;
	const float blockRight = blockLeft + gravityBlock.GetWidth();
	const float blockBottom = blockTop + gravityBlock.GetHeight();

	const bool isOverlapping =
		playerLeft < blockRight &&
		playerRight > blockLeft &&
		playerTop < blockBottom &&
		playerBottom > blockTop;

	if (!isOverlapping)
	{
		return false;
	}

	if (previousPlayerRight <= blockLeft && playerRight > blockLeft)
	{
		position_.x = blockLeft - width_;
		velocity_.x = 0.0f;
		knockbackVelocityX_ = 0.0f;
		EndDash();
		return true;
	}

	if (previousPlayerLeft >= blockRight && playerLeft < blockRight)
	{
		position_.x = blockRight;
		velocity_.x = 0.0f;
		knockbackVelocityX_ = 0.0f;
		EndDash();
		return true;
	}

	return false;
}

// 重力ブロックを、現在の重力方向に対する足場として扱う。
// 横方向が重なり、かつ縦方向も食い込んでいるときだけ、ブロックの面へ押し戻す。
bool Player::ResolveBlockVerticalCollision(
	const GravityBlock& gravityBlock,
	const GravitySystem& gravitySystem)
{
	if (isDashing_ || !gravityBlock.IsActive())
	{
		return false;
	}

	const float playerTop = position_.y;
	const float playerBottom = playerTop + height_;
	const float previousPlayerTop = previousPosition_.y;
	const float previousPlayerBottom = previousPlayerTop + height_;

	const float blockLeft = gravityBlock.GetPosition().x;
	const float blockTop = gravityBlock.GetPosition().y;
	const float blockRight = blockLeft + gravityBlock.GetWidth();
	const float blockBottom = blockTop + gravityBlock.GetHeight();

	// プレイヤーの中心寄りの幅がブロックに乗っているかを見る。
	// 端が少し触れただけで不自然に吸い寄せられるのを防ぐため。
	const float playerCenterX = position_.x + width_ * 0.5f;
	const float supportHalfWidth = fallHoleCheckWidth_ * 0.5f;
	const float supportLeft = playerCenterX - supportHalfWidth;
	const float supportRight = playerCenterX + supportHalfWidth;

	const bool isSupportedX =
		supportLeft < blockRight &&
		supportRight > blockLeft;

	const bool isOverlappingY =
		playerTop < blockBottom &&
		playerBottom > blockTop;

	if (!isSupportedX || !isOverlappingY)
	{
		return false;
	}

	if (gravitySystem.GetDirection() == GravityDirection::Down)
	{
		// 上向きに移動している最中は、下からブロックへ吸着しないようにする。
		if (velocity_.y < 0.0f)
		{
			return false;
		}

		const bool crossedBlockTop =
			previousPlayerBottom <= blockTop &&
			playerBottom >= blockTop;

		if (!crossedBlockTop)
		{
			return false;
		}

		position_.y = blockTop - height_;
	}
	else
	{
		// 下向きに移動している最中は、上からブロックへ吸着しないようにする。
		if (velocity_.y > 0.0f)
		{
			return false;
		}

		const bool crossedBlockBottom =
			previousPlayerTop >= blockBottom &&
			playerTop <= blockBottom;

		if (!crossedBlockBottom)
		{
			return false;
		}

		position_.y = blockBottom;
	}

	velocity_.y = 0.0f;
	isGrounded_ = true;
	UpdateState();

	return true;
}

float Player::GetWidth() const
{
	return width_;
}

float Player::GetHeight() const
{
	return height_;
}

// 外部から現在のプレイヤー状態を読み取るためのゲッター。
PlayerState Player::GetState() const
{
	return state_;
}

// 外部から現在の向きを読み取るためのゲッター。
FacingDirection Player::GetFacingDirection() const
{
	return facingDirection_;
}

int Player::GetHitPoint() const
{
	return hitPoint_;
}

int Player::GetMaxHitPoint() const
{
	return MaxHitPoint;
}

bool Player::IsDead() const
{
	return hitPoint_ <= 0;
}

bool Player::WasJumpedThisFrame() const
{
	return wasJumpedThisFrame_;
}

bool Player::WasDashStartedThisFrame() const
{
	return wasDashStartedThisFrame_;
}

