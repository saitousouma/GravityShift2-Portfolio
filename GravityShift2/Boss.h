#pragma once

#include <array>

#include "GravitySystem.h"
#include "BossSpawnData.h"
#include "MidBossSpawnData.h"
#include "Vector2.h"

struct StageData;
class Player;

// ステージに登場するボスの種類。
enum class BossType
{
	MidBoss, // ステージ4の中ボス。
	Boss     // ステージ5の戦車型大ボス。
};

// 中ボスと大ボスの移動、攻撃、飛び道具、ダメージを管理する。
class Boss
{
public:
	// 共通画像とボス種類別の画像を読み込む。
	bool Load();

	void Release();

	// 種類ごとの標準値、またはエディターの配置設定で初期化する。
	void Initialize(BossType type);

	void InitializeMidBoss(const MidBossSpawnData& spawnData);

	void InitializeBoss(const BossSpawnData& spawnData);

	// プレイヤーが戦闘範囲へ入った時点から行動を開始する。
	void StartEncounter();

	void Update(const GravitySystem& gravitySystem, const StageData& stageData,
		const Player& player);

	void Draw() const;

	// プレイヤーと攻撃判定を調べ、被弾元のX座標を返す。
	bool CheckProjectileCollision(
		const Player& player,
		float& damageSourceCenterX);

	// リスタート時に攻撃状態とHPを戦闘開始前へ戻す。
	void ResetAttackProjectiles();

	// 発生待ちの演出・効果音要求を一件ずつ取り出す。
	bool ConsumeExplosionEffect(Vector2& effectPosition);
	bool ConsumeGrenadeSoundRequest();
	bool ConsumeAttackWarningSoundRequest();

	// 無敵時間を設けずにHPを減らし、点滅表示だけを開始する。
	void TakeDamage();

	// ステージ切り替え時に、ボスと残っている攻撃を無効化する。
	void Deactivate();

	// プレイヤーの重力反転ではボス自身の重力を変更しない。
	void OnGravityReversed();

	bool IsActive() const;

	bool IsDefeated() const;

	bool HasEncounterStarted() const;

	const Vector2& GetPosition() const;

	float GetWidth() const;

	float GetHeight() const;
	int GetHitPoint() const;

private:
	// ボスが生成する攻撃オブジェクトの種類。
	enum class ProjectileType
	{
		Mine,              // 一定位置に残る機雷。
		Grenade,           // 地形上を転がるグレネード。
		RicochetGrenade,   // 壁で跳ね返るグレネード。
		TankShell,         // 戦車が発射する砲弾。
		Explosion          // 短時間だけ残る爆発判定。
	};

	// 大ボスが順番に選ぶ攻撃パターン。
	enum class TankAttackPattern
	{
		Cannon,
		Burst,
		GrenadeVolley,
		MineDrop,
		Count,
		AntiSurfaceShot
	};

	// 一個の飛び道具に必要な移動・寿命・状態を保持する。
	struct Projectile
	{
		Vector2 position{};
		Vector2 velocity{};
		ProjectileType type = ProjectileType::Mine;
		GravityDirection gravityDirection = GravityDirection::Down;
		int timer = 0;
		int remainingBounces = 0;
		bool isRolling = false;
		bool hasHitPlayer = false;
		bool isActive = false;
	};

	// ボス種類ごとの移動と攻撃判断を更新する。
	void UpdateMidBoss(const StageData& stageData, const Player& player);

	void UpdateTankBoss(const GravitySystem& gravitySystem,
		const StageData& stageData, const Player& player);

	// 中ボスの予告、攻撃発生、クールタイムを進める。
	void UpdateAttack();

	void UpdateTankAttack(const Player& player, bool playerOnOppositeSurface);
	void UpdateProjectiles(const StageData& stageData);

	// 中ボス用の攻撃を生成する。
	void SpawnMine();

	void SpawnGrenadeVolley();
	void SpawnPincerGrenadeVolley();
	void SpawnRicochetGrenadeVolley();

	// 大ボス用の攻撃を生成する。
	void SpawnTankShell();

	void SpawnTankAimedShell();

	void SpawnTankGrenadeVolley();

	void SpawnTankMines();

	// 空いている配列要素を使って飛び道具を生成する。
	void SpawnProjectile(ProjectileType type, const Vector2& center,
		const Vector2& velocity);
	void SpawnProjectileWithGravity(ProjectileType type, const Vector2& center,
		const Vector2& velocity, GravityDirection gravityDirection);

	// 元の飛び道具を消し、同じ位置に爆発判定と演出要求を作る。
	void ExplodeProjectile(Projectile& projectile);

	void ReverseOwnGravity();

	void ClearProjectiles();

	void DrawAttackWarning() const;

	void DrawProjectiles() const;

	// 飛び道具とステージ地形の接触を調べる補助処理。
	bool RectOverlapsSolid(const StageData& stageData,
		float left, float top, float width, float height) const;

	bool HasProjectileSupport(const Projectile& projectile,
		const StageData& stageData) const;

	// 指定された重力を加え、画面基準の床または天井との衝突を解決する補助処理。
	void ApplyGravity(const GravitySystem& gravitySystem);

	void ResolveVerticalCollision(const GravitySystem& gravitySystem);

	bool IsSolidAt(const StageData& stageData, int column, int row) const;

	// ボス本体の状態。
	Vector2 position_{};
	Vector2 velocity_{};
	BossType type_ = BossType::MidBoss;
	GravityDirection ownGravityDirection_ = GravityDirection::Down;
	EnemySpawnFacing facing_ = EnemySpawnFacing::Left;
	float width_ = static_cast<float>(MidBossSpawnData::Size);
	float height_ = static_cast<float>(MidBossSpawnData::Size);
	int hitPoint_ = MidBossSpawnData::DefaultHitPoint;
	int maxHitPoint_ = MidBossSpawnData::DefaultHitPoint;
	int hitFlashFrames_ = 0;
	int animationTimer_ = 0;
	int idleImageHandle_ = -1;
	int walkImageHandle_ = -1;
	int mineImageHandle_ = -1;
	int grenadeImageHandle_ = -1;
	int attackWarningImageHandle_ = -1;
	int tankMoveImageHandle_ = -1;
	int tankFireImageHandle_ = -1;
	// 現在の攻撃進行と予約されている攻撃を保持する。
	int attackCooldownFrames_ = 0;
	int attackWindupFrames_ = 0;
	int completedAttackCount_ = 0;
	bool pendingGrenadeAttack_ = false;
	bool pendingMineDashAttack_ = false;
	bool pendingPincerAttack_ = false;
	bool pendingRicochetAttack_ = false;
	bool grenadeSoundRequested_ = false;
	bool attackWarningSoundRequested_ = false;
	bool isMineDashActive_ = false;
	int mineDashDropTimer_ = 0;
	bool tankShotFired_ = false;
	TankAttackPattern tankAttackPattern_ = TankAttackPattern::Cannon;
	int tankShotsFired_ = 0;
	int tankGravityMismatchFrames_ = 0;
	int tankGravityFlipCooldownFrames_ = 0;
	Vector2 tankAttackTarget_{};
	// 飛び道具と、アプリケーションへ渡す爆発演出の待ち行列。
	std::array<Projectile, 24> projectiles_{};
	std::array<Vector2, 24> pendingExplosionEffects_{};
	int pendingExplosionEffectCount_ = 0;
	static constexpr int HitFlashDuration = 12;
	static constexpr int AttackCooldownDuration = 150;
	static constexpr int AttackWindupDuration = 45;
	static constexpr float MineDashSpeed = 5.0f;
	static constexpr int MineDashDropInterval = 36;
	static constexpr int AttackWarningFrameCount = 14;
	static constexpr int AttackWarningFrameSize = 80;
	static constexpr int TankMoveFrameCount = 6;
	static constexpr int TankFireFrameCount = 8;
	static constexpr int TankAnimationFrameDuration = 6;
	static constexpr int TankFireDuration =
		TankFireFrameCount * TankAnimationFrameDuration;
	static constexpr int TankFireFrame = 3;
	static constexpr int TankAttackCooldownDuration = 105;
	static constexpr int TankEnragedAttackCooldownDuration = 70;
	static constexpr int TankBurstIntervalDuration = 8;
	static constexpr int TankGravityFlipDelayDuration = 150;
	static constexpr int TankGravityFlipCooldownDuration = 120;
	bool isInitialized_ = false;
	bool hasEncounterStarted_ = false;
	bool isActive_ = false;
	bool isDefeated_ = false;
};
