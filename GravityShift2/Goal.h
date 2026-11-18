#pragma once

#include "Vector2.h"

class Player;

// 通常ステージで使用するゴール地点。
// プレイヤーが接触するとステージクリア条件を満たす。
class Goal
{
public:
	bool Load();
	void Release();

	// ゴールの位置を初期位置へ戻す。
	void Initialize();

	// ステージファイルで指定された位置へゴールを配置する。
	void Initialize(const Vector2& position);

	void Draw() const;

	// プレイヤーがゴール範囲に触れているか判定する。
	bool IsPlayerReached(const Player& player) const;

private:
	Vector2 position_{};
	float width_ = 48.0f;
	float height_ = 120.0f;
	int imageHandle_ = -1;

	static constexpr int FrameWidth = 80;
	static constexpr int FrameHeight = 80;
	static constexpr int FrameCount = 60;
	static constexpr int FrameDurationMilliseconds = 17;
};
