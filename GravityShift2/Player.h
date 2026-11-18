#pragma once

#include "Vector2.h"

class InputManager;
class GravitySystem;
class GravityBlock;
struct StageData;

// アニメーション状態。
enum class PlayerState
{
	Idle,
	Run,
	Dash,
	Jump,
	Shoot,
	Laser
};

// プレイヤーの向き。
// 数値は弾の発射方向にもそのまま使えるようにしている。
enum class FacingDirection
{
	Left = -1,
	Right = 1
};

class Player
{
public:
	void LoadSprites();
	void ReleaseSprites();
	void Initialize();
	void Initialize(const Vector2& startPosition);

	void Update(
		const InputManager& inputManager,
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	void Draw() const;
	void AdvanceDeathAnimation();

	void OnGravityReversed();

	void OnShoot();

	void SetLaserActive(bool isActive);

	// ステージかいしいちに戻す。
	void ResetToStartPosition();
	void ResetToStartPosition(const Vector2& startPosition);

	// ダメージを受けたときのHP減少、無敵時間開始、ノックバックを行う。
	bool TakeDamage(float damageSourceCenterX);

	// HPを指定量回復する。満タンまたは死亡中なら回復しない。
	bool Heal(int amount = 1);

	// レーザーを発射した瞬間に呼ばれる。
	void OnLaserFire();

	Vector2 GetShootPosition() const;
	Vector2 GetHeadShootPosition() const;
	Vector2 GetFootShootPosition() const;

	Vector2 GetLaserStartPosition() const;

	// プレイヤーの左上座標を取得。
	const Vector2& GetPosition() const;

	// 穴ループ用に、Y座標だけを指定位置へ移動する。
	void SetPositionY(float y);


	// 外部の押し戻し後に、ステージの壁へめり込んでいないか再確認する。
	void ResolveStageHorizontalCollision(const StageData& currentStage);

	// 重力ブロックに横方向から触れていたら、左右へ押し戻す。
	bool ResolveBlockHorizontalCollision(const GravityBlock& gravityBlock);

	// 重力ブロックに縦方向から触れていたら、床・天井のように接地させる。
	bool ResolveBlockVerticalCollision(
		const GravityBlock& gravityBlock,
		const GravitySystem& gravitySystem
	);

	float GetWidth() const;

	float GetHeight() const;

	PlayerState GetState() const;

	FacingDirection GetFacingDirection() const;

	int GetHitPoint() const;

	int GetMaxHitPoint() const;

	bool IsDead() const;

	bool WasJumpedThisFrame() const;
	bool WasDashStartedThisFrame() const;

private:
	Vector2 position_{};
	Vector2 previousPosition_{};
	Vector2 velocity_{};

	int shootStateTimer_ = 0;
	static constexpr int ShootStateDuration = 10;

	float width_ = 48.0f;
	float height_ = 64.0f;
	float moveSpeed_ = 5.0f;
	float jumpSpeed_ = 12.0f;
	float dashSpeed_ = 10.0f;
	static constexpr float DashDistance = 240.0f;
	static constexpr float FallAccelerationScale = 0.7f;
	float maxFallSpeed_ = 12.0f;
	float fallHoleCheckWidth_ = 32.0f;
	float knockbackVelocityX_ = 0.0f;
	float dashDistanceTravelled_ = 0.0f;
	float dashHeight_ = 0.0f;
	int dashDirection_ = 1;
	int dashCooldownFrames_ = 0;

	bool isGrounded_ = false;
	bool isDashing_ = false;
	bool isLaserActive_ = false;
	bool isGravityUp_ = false;
	bool wasJumpedThisFrame_ = false;
	bool wasDashStartedThisFrame_ = false;
	int animationTick_ = 0;
	int hurtAnimationTick_ = -1;
	int deathAnimationTick_ = 0;
	int idleSprite_ = -1;
	int runSprite_ = -1;
	int dashSprite_ = -1;
	int jumpSprite_ = -1;
	int jumpRiseSprite_ = -1;
	int shootSprite_ = -1;
	int hurtSprite_ = -1;
	int deathSprite_ = -1;

	static constexpr int MaxHitPoint = 5;
	static constexpr int InvincibleFrames = 90;
	static constexpr float DamageKnockbackSpeed = 9.0f;
	static constexpr float KnockbackDecay = 0.85f;
	// 180 frames is about three seconds at 60 fps.
	static constexpr int DashCooldownFrames = 180;

	int hitPoint_ = MaxHitPoint;
	int invincibleTimer_ = 0;

	PlayerState state_ = PlayerState::Idle;

	FacingDirection facingDirection_ = FacingDirection::Right;

	void UpdateState();
	void SetState(PlayerState state);

	// ダメージを受けた方向と反対側へ押し返す。
	void ApplyDamageKnockback(float damageSourceCenterX);
	
	void UpdateDash(const InputManager& inputManager);
	void StartDash(int direction);
	void EndDash();

	// 左右入力
	void UpdateHorizontalMovement(const InputManager& inputManager);

	// 壁や横方向の地形にめり込んだとき、左右へ押し戻す。
	void ResolveHorizontalCollision(const StageData& currentStage);

	// 地面にいるときのジャンプ入力
	void UpdateJump(
		const InputManager& inputManager,
		const GravitySystem& gravitySystem
	);

	// 重力の影響を受ける
	void ApplyGravity(const GravitySystem& gravitySystem);

	// 床と天井の衝突判定
	void ResolveVerticalCollision(
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	// タイルマップ上の'#'を足場として扱い、上下方向の接触を解決する。
	bool ResolveTileVerticalCollision(
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	// 穴として開いている床・天井タイルなら、当たり判定を無効にする。
	bool IsTileInsideFallHole(
		int tileLeft,
		int tileRight,
		int tileTop,
		const StageData& currentStage
	) const;
};
