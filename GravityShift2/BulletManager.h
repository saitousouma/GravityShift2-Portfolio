#pragma once

#include "Bullet.h"
#include <array>

class Enemy;
class Boss;
class GravityBlockManager;
struct StageData;

class BulletManager
{
public:
	void Shoot(const Vector2& position,
		const Vector2& direction);

	void Update(const StageData& currentStage,
		const GravityBlockManager& gravityBlockManager);

	void Draw() const;

	void Release();

	// ステージ切り替え時に、画面内の弾をすべて消す。
	void Clear();

	// 発射中の弾と敵の当たり判定を行う。
	void CheckCollision(Enemy& enemy);

	// 発射中の弾とボスの当たり判定を行う。
	void CheckCollision(Boss& boss);

	bool Load();

	int GetImageHandle() const;
private:
	static constexpr int MaxBullets = 32;

	std::array<Bullet, MaxBullets> bullets_{};

	int imageHandle_ = -1;
};
