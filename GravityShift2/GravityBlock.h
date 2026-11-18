#pragma once

#include "GravitySystem.h"
#include "GravityBlockSpawnData.h"
#include "Vector2.h"

struct StageData;

// 重力反転に合わせて、床と天井を行き来するブロック。
// 将来は敵を押しつぶすステージギミックとして使う。
class GravityBlock
{
public:
	// ブロックを初期位置へ配置する。
	void Initialize(
		const Vector2& position,
		GravityDirection gravityDirection = GravityDirection::Down,
		GravityBlockMotion motion = GravityBlockMotion::Gravity
	);

	// ブロックを未使用状態にする。
	void Deactivate();

	// 重力による落下、床・天井への接地、穴ループを更新する。
	void Update(
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	// 重力反転時に、ブロック自身の重力方向も反転する。
	void OnGravityReversed();

	// ブロックを描画する。
	void Draw() const;

	// ブロック画像に使う画像ハンドルを設定する。
	void SetImageHandle(int imageHandle);

	// ブロックの左上座標を返す。
	const Vector2& GetPosition() const;

	// 当たり判定用の横幅を返す。
	float GetWidth() const;

	// 当たり判定用の高さを返す。
	float GetHeight() const;

	// 現在の重力方向へ落下中かを返す。
	bool IsFalling() const;

	// プレイヤーへ接触ダメージを与える状態かを返す。
	bool IsDamaging() const;

	// このフレームで床または天井に着地したかを返す。
	bool WasLandedThisFrame() const;

	// ブロック自身の現在の重力方向を返す。
	GravityDirection GetGravityDirection() const;

	// ブロックが使用中かを返す。
	bool IsActive() const;

private:
	// 現在の重力方向へ速度を加える。
	void ApplyGravity(const GravitySystem& gravitySystem);

	// 床と天井の衝突判定を行う。
	void ResolveVerticalCollision(const StageData& currentStage);

	// タイルマップ上の'#'を足場として扱い、上下方向の接触を解決する。
	bool ResolveTileVerticalCollision(const StageData& currentStage);

	// 穴として開いている床・天井タイルなら、当たり判定を無効にする。
	bool IsTileInsideFallHole(
		int tileLeft,
		int tileRight,
		int tileTop,
		const StageData& currentStage
	) const;

	// ブロックの中心寄りの幅が、どれか1つの落下穴に入っているか調べる。
	bool IsOverFallHole(const StageData& currentStage) const;

	// 穴から画面外へ抜けたら、同じX位置の反対側へ出現させる。
	void CheckFallLoop(const StageData& currentStage);

	// 自動往復型の移動端を周囲の地形から決める。
	void InitializeMotionBounds(const StageData& currentStage);

	// 自動往復型を1フレーム更新する。
	void UpdateAutomaticMotion(const StageData& currentStage);

	Vector2 position_{};
	Vector2 previousPosition_{};
	Vector2 velocity_{};

	GravityDirection gravityDirection_ = GravityDirection::Down;
	GravityBlockMotion motion_ = GravityBlockMotion::Gravity;

	int imageHandle_ = -1;

	float width_ = 64.0f;
	float height_ = 64.0f;
	float maxFallSpeed_ = 18.0f;
	float motionMinimum_ = 0.0f;
	float motionMaximum_ = 0.0f;
	float automaticMoveSpeed_ = 12.0f;
	int automaticWaitFrames_ = 60;
	int automaticWaitTimer_ = 0;
	int automaticMoveDirection_ = -1;

	bool isActive_ = false;
	bool landedThisFrame_ = false;
	bool motionBoundsInitialized_ = false;
};
