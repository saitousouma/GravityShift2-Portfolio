#pragma once

enum class GravityDirection
{
	Down = 1, // 床方向。
	Up = -1   // 天井方向。
};

// 現在の重力方向と、物体へ加える加速度を管理する。
class GravitySystem
{
public:
	// 現在の重力方向を反転する。
	void Reverse();

	// 重力方向を初期状態の下向きに戻す。
	void Reset();
	// リスタートなどで指定方向へ直接戻す。
	void SetDirection(GravityDirection direction);

	GravityDirection GetDirection() const;
	float GetAcceleration() const;

private:
	GravityDirection direction_ = GravityDirection::Down;
	// 符号は重力方向から求めるため、加速度は正の大きさで保持する。
	float acceleration_ = 0.5f;
};
