#include "Bullet.h"
#include "GameConfig.h"
#include "StageData.h"
#include "DxLib.h"

// 弾を発射状態にする。縦射撃では当たり判定も縦長へ切り替える。
void Bullet::Initialize(const Vector2& position, const Vector2& direction)
{
	position_ = position;
	direction_ = direction;

	velocity_.x = speed_ * direction_.x;
	velocity_.y = speed_ * direction_.y;
	if (direction_.y != 0.0f)
	{
		width_ = VerticalWidth;
		height_ = VerticalHeight;
	}
	else
	{
		width_ = HorizontalWidth;
		height_ = HorizontalHeight;
	}

	isActive_ = true;
}

// 弾を1フレーム移動する。上下の範囲外判定は穴移動後にManagerが行う。
void Bullet::Update()
{
	if (!isActive_)
	{
		return;
	}

	position_.x += velocity_.x;
	position_.y += velocity_.y;


	if (position_.x + width_ < 0.0f ||
		position_.x > static_cast<float>(StageData::StagePixelWidth))
	{
		isActive_ = false;
	}
}

// 有効な弾だけを黄色い矩形として描画する。
// 後でスプライト弾へ差し替える予定の仮表示。
void Bullet::Draw() const
{
	if (!isActive_)
	{
		return;
	}

	const int left = static_cast<int>(position_.x);
	const int top = static_cast<int>(position_.y);
	const int right = static_cast<int>(position_.x + width_);
	const int bottom = static_cast<int>(position_.y + height_);

	if (imageHandle_ != -1)
	{
		const int sourceX = 192;
		const int sourceY = 32;
		const int sourceWidth = 64;
		const int sourceHeight = 64;
		constexpr double DrawScale = 0.75;
		const int centerX = static_cast<int>(position_.x + width_ * 0.5f);
		const int centerY = static_cast<int>(position_.y + height_ * 0.5f);

		if (direction_.y == 0.0f)
		{
			const int turnFlag = direction_.x < 0.0f ? TRUE : FALSE;
			DrawRectRotaGraph(
				centerX, centerY,
				sourceX, sourceY,
				sourceWidth, sourceHeight,
				DrawScale, 0.0, imageHandle_, TRUE, turnFlag, FALSE);
		}
		else
		{
			constexpr double HalfPi = 1.5707963267948966;
			const double angle = direction_.y < 0.0f ? -HalfPi : HalfPi;
			DrawRectRotaGraph(
				centerX, centerY,
				sourceX, sourceY,
				sourceWidth, sourceHeight,
				DrawScale, angle, imageHandle_, TRUE);
		}
	}
	else
	{
		DrawBox(
			left,
			top,
			right,
			bottom,
			GetColor(255, 230, 80),
			TRUE
		);
	}

}

// BulletManagerから共有画像ハンドルを受け取る。
// -1のままなら、従来通り矩形で仮描画する。
void Bullet::SetImageHandle(int imageHandle)
{
	imageHandle_ = imageHandle;
}

void Bullet::Deactivate()
{
	isActive_ = false;
}

const Vector2& Bullet::GetPosition() const
{
	return position_;
}

const Vector2& Bullet::GetDirection() const
{
	return direction_;
}

void Bullet::SetPositionY(float y)
{
	position_.y = y;
}

float Bullet::GetWidth() const
{
	return width_;
}

float Bullet::GetHeight() const
{
	return height_;
}

// 弾が現在使われているかを返す。
// BulletManagerが空き弾を探すために使う。
bool Bullet::IsActive() const
{
	return isActive_;
}


