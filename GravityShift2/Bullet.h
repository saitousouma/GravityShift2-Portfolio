#pragma once

#include "Vector2.h"

class Bullet
{
public:
	static constexpr float HorizontalWidth = 48.0f;
	static constexpr float HorizontalHeight = 24.0f;
	static constexpr float VerticalWidth = 24.0f;
	static constexpr float VerticalHeight = 48.0f;

	void Initialize(const Vector2& position, const Vector2& direction);

	void Update();

	void Draw() const;

	void SetImageHandle(int imageHandle);

	// 命中した弾を未使用状態に戻す。
	void Deactivate();

	// 弾の左上座標を返す。
	const Vector2& GetPosition() const;
	const Vector2& GetDirection() const;

	// 対になった穴を通過したとき、反対側の出口へ移す。
	void SetPositionY(float y);

	// 当たり判定用の横幅を返す。
	float GetWidth() const;

	// 当たり判定用の高さを返す。
	float GetHeight() const;

	bool IsActive() const;

private:
	Vector2 position_{};
	Vector2 velocity_{};

	float width_ = HorizontalWidth;
	float height_ = HorizontalHeight;
	float speed_ = 12.0f;

	// 弾の上下左右の移動方向。
	Vector2 direction_{ 1.0f, 0.0f };

	int imageHandle_ = -1;

	bool isActive_ = false;
};
