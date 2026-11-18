#include "ScorePopup.h"
#include "UiFont.h"
#include "DxLib.h"

void ScorePopup::Start(const Vector2& position, int score,
	int fontSize, int color)
{
	position_ = position;
	score_ = score;
	fontSize_ = fontSize;
	color_ = color;

	// 最初に少し上方向に跳ねる。
	velocity_.x = 0.0f;
	velocity_.y = JumpVelocityY;

	lifeTimer_ = LifeFrames;
	isActive_ = true;
}

void ScorePopup::Update()
{
	if (!isActive_)
	{
		return;
	}

	// 速度を位置に反映。
	position_.x += velocity_.x;
	position_.y += velocity_.y;

	// 下向きの重力を加える。
	velocity_.y += Gravity;

	--lifeTimer_;

	if (lifeTimer_ <= 0)
	{
		isActive_ = false;
	}
}

void ScorePopup::Draw() const
{
	if (!isActive_)
	{
		return;
	}

	// 現在のフォントサイズを保存してから、ポップアップ用のサイズに変える。
	const int previousFontSize = GetFontSize();

	SetFontSize(fontSize_);

	UiFont::DrawFormatStringSized(
		static_cast<int>(position_.x),
		static_cast<int>(position_.y),
		color_,
		fontSize_,
		L"+%d",
		score_
	);

	// 他のUIの邪魔をしないように、元のサイズに戻す。
	SetFontSize(previousFontSize);
}

bool ScorePopup::IsActive() const
{
	return isActive_;
}
