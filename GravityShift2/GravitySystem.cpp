#include "GravitySystem.h"

// 現在の重力方向を上下反転する。
// プレイヤーや敵は、この方向を参照して落下方向を決める。
void GravitySystem::Reverse()
{
	if (direction_ == GravityDirection::Down)
	{
		direction_ = GravityDirection::Up;
	}
	else
	{
		direction_ = GravityDirection::Down;
	}
}

// 重力方向を下向きに戻す。
void GravitySystem::Reset()
{
	direction_ = GravityDirection::Down;
}

void GravitySystem::SetDirection(GravityDirection direction)
{
	direction_ = direction;
}
// 現在の重力方向を返す。
GravityDirection GravitySystem::GetDirection() const
{
	return direction_;
}

// 方向を反映した重力加速度を返す。
// Downなら正、Upなら負の値になる。
float GravitySystem::GetAcceleration() const
{
	return acceleration_ * static_cast<int>(direction_);
}
