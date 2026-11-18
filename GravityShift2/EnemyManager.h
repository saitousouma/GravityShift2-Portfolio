#pragma once

#include "Enemy.h"
#include "EnemySpawnData.h"
#include "StageData.h"
#include <array>

class GravitySystem;
class BulletManager;
class Laser;
class EffectManager;
class ScoreManager;
class Player;
class GravityBlock;
class EnemyBulletManager;

class EnemyManager
{
public:
	// ステージ番号に応じて敵を初期位置へ配置。
	void Initialize(int stageNumber);

	// ステージデータに書かれた敵配置を使って敵を初期位置へ配置。
	void Initialize(const StageData& currentStage);

	// すべての敵を更新する。
	void Update(
		const GravitySystem& gravitySystem,
		const StageData& currentStage,
		Player& player,
		EnemyBulletManager& enemyBulletManager,
		int cameraX,
		int cameraY
	);

	// すべての敵を描画。
	void Draw() const;

	// すべての敵へ重力反転を通知。
	void OnGravityReversed();

	// すべての敵と弾とレーザーの当たり判定を行う。
	void CheckWeaponCollisions(
		BulletManager& bulletManager,
		Laser& laser,
		EffectManager& effectManager,
		ScoreManager& scoreManager
	);

	// 重力ブロックと敵の当たり判定を行う。
	void CheckGravityBlockCollisions(
		const GravityBlock& gravityBlock,
		EffectManager& effectManager,
		ScoreManager& scoreManager
	);

	// プレイヤーが有効な敵に触れているか調べ、当たった敵の中心Xを返す。
	bool CheckPlayerCollision(
		const Player& player,
		float& damageSourceCenterX
	) const;

	// 管理してる敵がすべて倒されているか調べる。
	bool IsAllDefeated() const;

	// 敵画像を読み込む。
	// 画像が無い場合でもゲームは続行し、矩形描画を使う。
	bool Load();

	bool ConsumeDamageSoundRequest();
	bool ConsumeDeathSoundRequest();

private:
	struct HealthPickup
	{
		Vector2 position{};
		float velocityY = 0.0f;
		bool isActive = false;
	};

	static constexpr int MaxEnemies = StageData::MaxEnemySpawns;
	static constexpr int EnemyActivationMargin = 200;
	static constexpr int HealthPickupDropChancePercent = 80;
	static constexpr float HealthPickupWidth = 32.0f;
	static constexpr float HealthPickupHeight = 34.0f;
	static constexpr float HealthPickupGravity = 0.3f;
	static constexpr float HealthPickupMaxFallSpeed = 6.0f;

	// カメラの周囲にいる敵だけを動かすための判定。
	bool IsEnemyInsideActivationArea(
		const Enemy& enemy,
		int cameraX,
		int cameraY
	) const;
	void ClearHealthPickups();
	void TrySpawnHealthPickup(const Enemy& enemy);
	void UpdateHealthPickups(
		const GravitySystem& gravitySystem,
		const StageData& currentStage,
		Player& player);
	void DrawHealthPickups() const;
	bool IsHealthPickupTouchingSolid(
		const HealthPickup& pickup,
		float nextY,
		const StageData& currentStage,
		int& collisionRow) const;

	// 敵一体分の出現データを設定する。
	void AddSpawnData(
		std::array<EnemySpawnData, MaxEnemies>& spawnData,
		int& enemyCount,
		float x,
		int hitPoint,
		EnemySpawnSurface surface = EnemySpawnSurface::Floor,
		EnemyType type = EnemyType::Normal
	) const;
	
	// 敵タイプごとの画像ハンドル。
	// -1の場合はEnemy側で矩形描画に戻る。
	int normalEnemyImageHandle_ = -1;
	int heavyEnemyImageHandle_ = -1;
	int shooterEnemyImageHandle_ = -1;
	int chargerEnemyImageHandle_ = -1;
	int mortarEnemyImageHandle_ = -1;
	int healthPickupImageHandle_ = -1;

	std::array<Enemy, MaxEnemies> enemies_{};
	std::array<HealthPickup, MaxEnemies> healthPickups_{};
	bool damageSoundRequested_ = false;
	bool deathSoundRequested_ = false;
};
