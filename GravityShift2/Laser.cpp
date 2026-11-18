#include "Laser.h"
#include "Enemy.h"
#include "Boss.h"
#include "DxLib.h"

// レーザーの有効状態、開始位置、向きを毎フレーム更新する。
// レーザーは弾と違い、発射後に独立せずプレイヤーに追従する。
void Laser::Update(
	bool isFireTriggered,
	const Vector2& startPosition,
	float direction)
{
	wasFiredThisFrame_ = false;

	// クールタイム中なら、毎フレーム残り時間を減らす。
	if (remainingCooldownFrames_ > 0)
	{
		--remainingCooldownFrames_;
	}

	// 照射中なら、プレイヤーの現在位置と向きに追従しながら残り時間を減らす。
	if (isActive_)
	{
		startPosition_ = startPosition;
		direction_ = direction;

		if (damageIntervalTimer_ > 0)
		{
			--damageIntervalTimer_;
		}

		--remainingActiveFrames_;

		if (remainingActiveFrames_ <= 0)
		{
			isActive_ = false;
			remainingActiveFrames_ = 0;

			// 照射が終わった瞬間にクールタイム開始。
			remainingCooldownFrames_ = CooldownFrames;
		}

		return;
	}

	// 照射中でもクールタイム中でもない時だけ、Xを押した瞬間に発射できる。
	if (isFireTriggered && remainingCooldownFrames_ <= 0)
	{
		isActive_ = true;
		wasFiredThisFrame_ = true;
		remainingActiveFrames_ = ActiveDurationFrames;

		damageIntervalTimer_ = 0;

		startPosition_ = startPosition;
		direction_ = direction;
	}
}

// 照射中のレーザーを横長の矩形として描画する。
// direction_が右なら右方向、左なら左方向へ伸ばす。
void Laser::Draw() const
{
	if (!isActive_)
	{
		return;
	}

	const float endX =
		startPosition_.x + static_cast<float>(length_) * direction_;

	const int left = static_cast<int>(
		direction_ > 0.0f ? startPosition_.x : endX
		);

	const int right = static_cast<int>(
		direction_ > 0.0f ? endX : startPosition_.x
		);

	const int top = static_cast<int>(
		startPosition_.y - height_ * 0.5f
		);

	const int bottom = static_cast<int>(
		startPosition_.y + height_ * 0.5f
		);

	const int drawY = static_cast<int>(
		startPosition_.y - 16.0f
		);

	if (imageHandle_ != -1)
	{
		const int segmentWidth = 192;
		const int segmentHeight = 32;
		const int segmentCount =
			(length_ + segmentWidth - 1) / segmentWidth;

		for (int i = 0; i < segmentCount; i++)
		{
			const int drawnLength = segmentWidth * i;
			const int remainingLength = length_ - drawnLength;

			const int drawWidth =
				remainingLength < segmentWidth
				? remainingLength
				: segmentWidth;

			const int segmentX = static_cast<int>(
				direction_ > 0.0f
				? startPosition_.x + drawnLength
				: startPosition_.x - drawnLength - drawWidth
				);

			DrawRectGraph(
				segmentX,
				drawY,
				192,
				512,
				drawWidth,
				segmentHeight,
				imageHandle_,
				TRUE,
				direction_ < 0.0f
			);
		}
	}
	else
	{
		DrawBox(
			left,
			top,
			right,
			bottom,
			GetColor(80, 255, 255),
			TRUE
		);
	}
}

void Laser::SetImageHandle(int imagehandle)
{
	imageHandle_ = imagehandle;
}

// ステージ切り替え時に、レーザーの照射状態とクールタイムを初期化する。
void Laser::Reset()
{
	isActive_ = false;
	wasFiredThisFrame_ = false;
	remainingActiveFrames_ = 0;
	remainingCooldownFrames_ = 0;
	damageIntervalTimer_ = 0;
	startPosition_ = {};
	direction_ = 1.0f;
}

// レーザー全体を横長の矩形として扱い、敵との重なりを調べる。
bool Laser::CheckCollision(Enemy& enemy)
{
	if (!isActive_ || !enemy.IsActive())
	{
		return false;
	}

	const float endX =
		startPosition_.x + static_cast<float>(length_) * direction_;

	const float laserLeft =
		direction_ > 0.0f ? startPosition_.x : endX;

	const float laserRight =
		direction_ > 0.0f ? endX : startPosition_.x;

	const float laserTop =
		startPosition_.y - height_ * 0.5f;

	const float laserBottom =
		startPosition_.y + height_ * 0.5f;

	const Vector2& enemyPosition = enemy.GetPosition();

	const float enemyLeft = enemyPosition.x;
	const float enemyTop = enemyPosition.y;
	const float enemyRight = enemyLeft + enemy.GetWidth();
	const float enemyBottom = enemyTop + enemy.GetHeight();

	const bool isHit =
		laserLeft < enemyRight &&
		laserRight > enemyLeft &&
		laserTop < enemyBottom &&
		laserBottom > enemyTop;

	if (isHit && CanDealDamage())
	{
		enemy.TakeDamage();
		return true;
	}

	return false;
}

// レーザー全体を横長の矩形として扱い、ボスとの重なりを調べる。
bool Laser::CheckCollision(Boss& boss)
{
	if (!isActive_ || !boss.IsActive())
	{
		return false;
	}

	const float endX =
		startPosition_.x + static_cast<float>(length_) * direction_;

	const float laserLeft =
		direction_ > 0.0f ? startPosition_.x : endX;

	const float laserRight =
		direction_ > 0.0f ? endX : startPosition_.x;

	const float laserTop =
		startPosition_.y - height_ * 0.5f;

	const float laserBottom =
		startPosition_.y + height_ * 0.5f;

	const Vector2& bossPosition = boss.GetPosition();

	const float bossLeft = bossPosition.x;
	const float bossTop = bossPosition.y;
	const float bossRight = bossLeft + boss.GetWidth();
	const float bossBottom = bossTop + boss.GetHeight();

	const bool isHit =
		laserLeft < bossRight &&
		laserRight > bossLeft &&
		laserTop < bossBottom &&
		laserBottom > bossTop;

	if (isHit && CanDealDamage())
	{
		boss.TakeDamage();
		return true;
	}

	return false;
}


// レーザーが照射中かどうかを返す。
bool Laser::IsActive() const
{
	return isActive_;
}

int Laser::GetRemainingActiveFrames() const
{
	return remainingActiveFrames_;
}

int Laser::GetRemainingCooldownFrames() const
{
	return remainingCooldownFrames_;
}

bool Laser::CanFire() const
{
	return !isActive_ && remainingCooldownFrames_ <= 0;
}

bool Laser::WasFiredThisFrame() const
{
	return wasFiredThisFrame_;
}

bool Laser::CanDealDamage() const
{
	return isActive_ && damageIntervalTimer_ <= 0;
}

void Laser::OnDamageDealt()
{
	damageIntervalTimer_ = DamageIntervalFrames;
}


float Laser::GetActiveRatio() const
{
	return static_cast<float>(remainingActiveFrames_)
		/ static_cast<float>(ActiveDurationFrames);
}

float Laser::GetCooldownRatio() const
{
	if (CooldownFrames <= 0)
	{
		return 0.0f;
	}

	return static_cast<float>(remainingCooldownFrames_)
		/ static_cast<float>(CooldownFrames);
}
