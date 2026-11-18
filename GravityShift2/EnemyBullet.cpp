#include "EnemyBullet.h"

#include "GameConfig.h"
#include "StageData.h"
#include "DxLib.h"

void EnemyBullet::Initialize(
	const Vector2& centerPosition,
	const Vector2& velocity,
	EnemyBulletType type)
{
	type_ = type;
	// 縦弾と分裂後の横弾で、画像と当たり判定の向きを切り替える。
	if (type_ == EnemyBulletType::MortarVertical)
	{
		width_ = 10.0f;
		height_ = 26.0f;
	}
	else if (type_ == EnemyBulletType::MortarHorizontal)
	{
		width_ = 26.0f;
		height_ = 10.0f;
	}
	else
	{
		width_ = 24.0f;
		height_ = 24.0f;
	}
	position_.x = centerPosition.x - width_ * 0.5f;
	position_.y = centerPosition.y - height_ * 0.5f;
	velocity_ = velocity;
	isActive_ = true;
}

void EnemyBullet::Update()
{
	if (!isActive_)
	{
		return;
	}

	position_.x += velocity_.x;
	position_.y += velocity_.y;

	// ループ弾だけは上下のステージ外へ進んでから反対側へ移動する。
	const bool isOutsideHorizontal =
		position_.x + width_ < 0.0f ||
		position_.x > static_cast<float>(StageData::StagePixelWidth);
	const bool isOutsideVertical =
		position_.y + height_ < 0.0f ||
		position_.y > static_cast<float>(StageData::MaxStagePixelHeight);

	if (isOutsideHorizontal ||
		(type_ != EnemyBulletType::VerticalLoop && isOutsideVertical))
	{
		Deactivate();
	}
}

void EnemyBullet::Draw(int standardImageHandle, int mortarImageHandle) const
{
	if (!isActive_)
	{
		return;
	}

	const int drawX = static_cast<int>(position_.x);
	const int drawY = static_cast<int>(position_.y);

	// 固定砲台の弾は進行方向に合わせて画像を回転する。
	const bool usesMortarImage =
		type_ == EnemyBulletType::MortarVertical ||
		type_ == EnemyBulletType::MortarHorizontal;
	if (usesMortarImage && mortarImageHandle != -1)
	{
		constexpr double HalfPi = 1.5707963267948966;
		constexpr double Pi = 3.1415926535897932;
		double angle = velocity_.y > 0.0f ? Pi : 0.0;
		if (velocity_.x > 0.0f)
		{
			angle = HalfPi;
		}
		else if (velocity_.x < 0.0f)
		{
			angle = -HalfPi;
		}
		DrawRotaGraph(
			static_cast<int>(position_.x + width_ * 0.5f),
			static_cast<int>(position_.y + height_ * 0.5f),
			1.0, angle, mortarImageHandle, TRUE);
		return;
	}

	if (standardImageHandle != -1)
	{
		// DrawCircleはカメラ変換に追従しないため、画像で描画する。
		DrawGraph(drawX, drawY, standardImageHandle, TRUE);
		return;
	}

	// 画像の読み込みに失敗しても確認できるよう、代わりに矩形を描画する。
	DrawBox(drawX, drawY,
		drawX + static_cast<int>(width_) - 1,
		drawY + static_cast<int>(height_) - 1,
		GetColor(255, 80, 80), TRUE);
}

void EnemyBullet::Deactivate()
{
	isActive_ = false;
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
}

bool EnemyBullet::IsActive() const
{
	return isActive_;
}

const Vector2& EnemyBullet::GetPosition() const
{
	return position_;
}

void EnemyBullet::SetPositionY(float y)
{
	position_.y = y;
}

const Vector2& EnemyBullet::GetVelocity() const
{
	return velocity_;
}

EnemyBulletType EnemyBullet::GetType() const
{
	return type_;
}

float EnemyBullet::GetWidth() const
{
	return width_;
}

float EnemyBullet::GetHeight() const
{
	return height_;
}

