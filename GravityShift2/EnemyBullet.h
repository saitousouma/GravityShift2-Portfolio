#pragma once

#include "Vector2.h"

enum class EnemyBulletType
{
	Standard,         // プレイヤー方向へ進む通常弾。
	VerticalLoop,     // 上下の穴を通って反対側へ出る弾。
	MortarVertical,   // 床または天井に当たると分裂する弾。
	MortarHorizontal, // 分裂後に左右へ進む弾。
};

// 敵が発射する弾。
// プレイヤー弾とは当たる相手が違うので、別クラスとして管理する。
class EnemyBullet
{
public:
	// 弾を指定した中心位置から発射する。
	void Initialize(const Vector2& centerPosition, const Vector2& velocity,
		EnemyBulletType type = EnemyBulletType::Standard);

	// 弾を1フレーム分移動し、画面外に出たら無効化する。
	void Update();

	// 敵弾を描画する。
	void Draw(int standardImageHandle, int mortarImageHandle) const;

	// 弾を消す。
	void Deactivate();

	// 弾が現在有効かを返す。
	bool IsActive() const;

	// 弾の左上座標を返す。
	const Vector2& GetPosition() const;
	// 穴を通過したとき、反対側の出口へY座標を移す。
	void SetPositionY(float y);
	const Vector2& GetVelocity() const;
	EnemyBulletType GetType() const;

	// 当たり判定用の横幅を返す。
	float GetWidth() const;

	// 当たり判定用の高さを返す。
	float GetHeight() const;

private:
	Vector2 position_{};
	Vector2 velocity_{};

	float width_ = 24.0f;
	float height_ = 24.0f;
	EnemyBulletType type_ = EnemyBulletType::Standard;

	bool isActive_ = false;
};
