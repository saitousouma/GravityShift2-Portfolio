#include "Goal.h"
#include "Player.h"
#include "GameConfig.h"
#include "DxLib.h"

bool Goal::Load()
{
	imageHandle_ = LoadGraph(L"Assets/Images/Objects/goal-animation.png");
	if (imageHandle_ == -1)
	{
		imageHandle_ =
			LoadGraph(L"../../Assets/Images/Objects/goal-animation.png");
	}
	return true;
}

void Goal::Release()
{
	if (imageHandle_ != -1)
	{
		DeleteGraph(imageHandle_);
		imageHandle_ = -1;
	}
}

void Goal::Initialize()
{
	position_.x = static_cast<float>(GameConfig::ScreenWidth) - 120.0f;
	position_.y = static_cast<float>(GameConfig::FloorY) - height_;
}

void Goal::Initialize(const Vector2& position)
{
	position_ = position;
}

void Goal::Draw() const
{
	if (imageHandle_ != -1)
	{
		const int frame =
			(GetNowCount() / FrameDurationMilliseconds) % FrameCount;
		const int drawX = static_cast<int>(
			position_.x + (width_ - FrameWidth) * 0.5f);
		const int drawY = static_cast<int>(
			position_.y + height_ - FrameHeight);
		DrawRectGraph(
			drawX,
			drawY,
			frame * FrameWidth,
			0,
			FrameWidth,
			FrameHeight,
			imageHandle_,
			TRUE
		);
		return;
	}

	DrawBox(
		static_cast<int>(position_.x),
		static_cast<int>(position_.y),
		static_cast<int>(position_.x + width_),
		static_cast<int>(position_.y + height_),
		GetColor(255, 230, 80),
		TRUE
	);
}

// プレイヤーとゴールの矩形が重なっているか判定する。
bool Goal::IsPlayerReached(const Player& player) const
{
	const Vector2& playerPosition = player.GetPosition();

	const float playerLeft = playerPosition.x;
	const float playerTop = playerPosition.y;
	const float playerRight = playerPosition.x + player.GetWidth();
	const float playerBottom = playerPosition.y + player.GetHeight();

	const float goalLeft = position_.x;
	const float goalTop = position_.y;
	const float goalRight = position_.x + width_;
	const float goalBottom = position_.y + height_;

	const bool isOverlapping =
		playerLeft < goalRight &&
		playerRight > goalLeft &&
		playerTop < goalBottom &&
		playerBottom > goalTop;

	return isOverlapping;
}
