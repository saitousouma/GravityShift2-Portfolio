#pragma once

#include "Vector2.h"

class Enemy;
class Boss;

class Laser
{
public:
	// 開始位置と向きを更新する。
	void Update(
		bool isFireTriggered,
		const Vector2& startPosition,
		float direction
	);

	// レーザー本体を描画する。
	void Draw() const;

	void SetImageHandle(int imageHandle);

	// ステージ切り替え時に、レーザーの照射状態とクールタイムを初期化する。
	void Reset();

	// 照射中のレーザーと敵の当たり判定を行う。
	bool CheckCollision(Enemy& enemy);

	// 照射中のレーザーとボスの当たり判定を行う。
	bool CheckCollision(Boss& boss);

	// ダメージを与えた直後に呼び、次のダメージまでの待ち時間を作る。
	void OnDamageDealt();


	// レーザー照射中か確認する。
	bool IsActive() const;

	// 照射残りフレーム数を返す。
	int GetRemainingActiveFrames() const;

	// クールタイム残りフレーム数を返す。
	int GetRemainingCooldownFrames() const;

	// 今レーザーを発射できる状態か確認する。
	bool CanFire() const;

	// 直近のUpdateでレーザー発射に成功したか。
	bool WasFiredThisFrame() const;

	// 今このフレームでレーザーダメージを与えられるか確認する。
	bool CanDealDamage() const;


	// 照射時間を0.0～1.0の割合で返す。
	float GetActiveRatio() const;

	// クールタイムを0.0～1.0の割合で返す。
	float GetCooldownRatio() const;

private:

	Vector2 startPosition_{};

	float direction_ = 1.0f;
	float height_ = 10.0f;

	int length_ = 400;
	int imageHandle_ = -1;

	// 現在の照射残り時間。
	int remainingActiveFrames_ = 0;

	// 現在のクールタイム残り時間。
	int remainingCooldownFrames_ = 0;

	// 次にダメージを与えられるまでの待ち時間。
	int damageIntervalTimer_ = 0;

	// レーザーの照射時間。
	static constexpr int ActiveDurationFrames = 120;

	// クールタイム。60fpsなら300フレームで約5秒。
	static constexpr int CooldownFrames = 300;

	// レーザーがダメージを与えられる間隔。
	static constexpr int DamageIntervalFrames = 10;

	// 直近のUpdateでレーザー発射に成功したか。
	bool wasFiredThisFrame_ = false;

	bool isActive_ = false;


};