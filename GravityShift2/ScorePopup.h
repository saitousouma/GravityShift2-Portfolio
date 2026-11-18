#pragma once
#include "Vector2.h"

// 敵を倒した時のスコア表示
class ScorePopup
{
public:

	void Start(const Vector2& position, int score, int fontSize,
		int color);

	void Update();

	void Draw() const;

	bool IsActive() const;

private:

	Vector2 position_{};
	Vector2 velocity_{};

	int score_ = 0;
	int lifeTimer_ = 0;
	int fontSize_ = 20;
	int color_ = 0;

	bool isActive_ = false;

	// 表示される時間。
	static constexpr int LifeFrames = 60;

	// 表示開始時に上へ跳ねる力。
	static constexpr float JumpVelocityY = -4.0f;

	// ポップアップにかかる下向きの重力。
	static constexpr float Gravity = 0.15f;

};
