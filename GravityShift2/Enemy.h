#pragma once

#include "Vector2.h"
#include "GravitySystem.h"
#include "EnemyType.h"

struct StageData;
class Player;
class EnemyBulletManager;

class Enemy
{
public:
	// 敵を指定位置に配置する。
	void Initialize(
		const Vector2& position,
		int hitPoint = 1,
		GravityDirection gravityDirection = GravityDirection::Down,
		EnemyType type = EnemyType::Normal,
		bool isAffectedByGravityReversal = true,
		float horizontalDirection = -1.0f
	);

	// 重力、移動、射撃、ステージとの当たり判定を更新する。
	void Update(
		const GravitySystem& gravitySystem,
		const StageData& currentStage,
		const Player& player,
		EnemyBulletManager& enemyBulletManager
	);

	// 重力反転時に、反転前の落下速度をリセットする。
	void OnGravityReversed();

	// 敵を描画する。
	void Draw() const;

	// 敵画像に使う画像ハンドルを設定する。
	// 画像が読み込まれていない場合は、Draw内で矩形描画に戻す。
	void SetImageHandles(
		int normalImageHandle,
		int heavyImageHandle,
		int shooterImageHandle,
		int chargerImageHandle,
		int mortarImageHandle
	);

	// ダメージを受けた敵を無効化する。
	void TakeDamage();

	int GetHitPoint() const;

	// 敵を即座に無効化する。
	void Deactivate();

	// 敵の左上座標を返す。
	const Vector2& GetPosition() const;

	// 当たり判定用の横幅を返す。
	float GetWidth() const;

	// 当たり判定用の高さを返す。
	float GetHeight() const;

	// 敵が現在有効かを返す。
	bool IsActive() const;

	// 敵タイプごとの横幅を返す。
	static float GetWidthByType(EnemyType type);

	// 敵タイプごとの縦幅を返す。
	static float GetHeightByType(EnemyType type);

	// 敵の種類を返す。
	EnemyType GetType() const;

private:
	// 現在の重力方向へ落下速度を加える。
	void ApplyGravity(const GravitySystem& gravitySystem);
	// 横移動後に壁へ入った場合、壁面まで戻して通常敵は反転、突進敵は停止する。
	void ResolveTileHorizontalCollision(const StageData& currentStage);

	// 床と天井の衝突判定を行う。
	void ResolveVerticalCollision(
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	// タイルマップ上の'#'を足場として扱い、上下方向の接触を解決する。
	bool ResolveTileVerticalCollision(const StageData& currentStage);

	// 穴として開いている床・天井タイルなら、当たり判定を無効にする。
	bool IsTileInsideFallHole(
		int tileLeft,
		int tileRight,
		int tileTop,
		const StageData& currentStage
	) const;

	// 敵を左右へ移動させ、画面端で向きを反転する。
	void UpdateHorizontalMovement();

	// 敵の足元判定幅が、どれか1つの落下穴に入っているか調べる。
	bool IsOverFallHole(const StageData& currentStage) const;

	// 穴から画面外へ抜けたら、同じX位置の反対側へ出現させる。
	void CheckFallLoop(const StageData& currentStage);

	// プレイヤーが検知範囲内にいるか調べる。
	bool CanDetectPlayer(const Player& player, float detectionRange) const;

	// 射撃タイプの敵だけが行う、検知と射撃の処理。
	void UpdateShooterAttack(
		const Player& player,
		EnemyBulletManager& enemyBulletManager
	);
	void UpdateCharger(const Player& player);
	void UpdateMortar(const GravitySystem& gravitySystem,
		EnemyBulletManager& enemyBulletManager);

	// 敵の中心座標を返す。
	Vector2 GetCenterPosition() const;

	Vector2 position_{};
	Vector2 previousPosition_{};
	Vector2 velocity_{};

	GravityDirection gravityDirection_ = GravityDirection::Down;
	EnemyType type_ = EnemyType::Normal;
	bool isAffectedByGravityReversal_ = true;

	// 敵タイプごとの画像ハンドル。
	// -1の場合は画像なしとして、仮の矩形描画を使う。
	int normalImageHandle_ = -1;
	int heavyImageHandle_ = -1;
	int shooterImageHandle_ = -1;
	int chargerImageHandle_ = -1;
	int mortarImageHandle_ = -1;

	int hitPoint_ = 1;
	int damageCooldownFrames_ = 0;
	int shootCooldownFrames_ = 0;
	int walkAnimationFrame_ = 0;
	int walkAnimationTimer_ = 0;
	int shooterAnimationFrame_ = 0;
	int shooterAnimationTimer_ = 0;
	bool isShooterAnimating_ = false;
	int chargerAnimationFrame_ = 0;
	int chargerAnimationTimer_ = 0;
	bool isChargerActivated_ = false;
	bool isChargerStopped_ = false;

	static constexpr int DamageCooldownDuration = 30;
	static constexpr int WalkAnimationFrameCount = 8;
	static constexpr int WalkAnimationFrameDuration = 6;
	static constexpr int ShooterAttackInterval = 90;
	static constexpr int ShooterWarningFrames = 30;
	static constexpr int ShooterAnimationFrameCount = 8;
	static constexpr int ShooterAnimationFrameDuration = 5;
	static constexpr int ShooterFireFrame = 6;
	static constexpr float ShooterDetectionRange = 760.0f;
	static constexpr int ChargerAnimationFrameCount = 10;
	static constexpr int ChargerAnimationFrameDuration = 4;
	static constexpr float ChargerDetectionRange = 720.0f;
	static constexpr float ChargerMoveSpeed = 8.0f;
	static constexpr int MortarAttackCooldown = 60 * 3;

	float width_ = 48.0f;
	float height_ = 48.0f;

	// 横方向の移動速度。
	float moveSpeed_ = 2.0f;

	// 落下速度が上がり続けないようにするための最大速度。
	static constexpr float FallAccelerationScale = 0.7f;
	float maxFallSpeed_ = 12.0f;

	// ダメージを受けたときに一時的に加える横方向の押し戻し速度。
	float knockbackVelocityX_ = 0.0f;

	// 1.0なら右、-1.0なら左へ進む。
	float horizontalDirection_ = -1.0f;

	bool isActive_ = true;
};
