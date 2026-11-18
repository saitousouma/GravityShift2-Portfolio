#pragma once

#include <array>

#include "GravityBlock.h"
#include "StageData.h"
#include "Vector2.h"

class EnemyManager;
class EnemyBullet;
class Bullet;
class EffectManager;
class GravitySystem;
class Player;
class ScoreManager;

// 複数の重力ブロックをまとめて管理するクラス。
// StageDataに登録された配置を実体化し、更新、描画、敵との押しつぶし判定を担当する。
class GravityBlockManager
{
public:
	// 重力ブロック画像を読み込む。
	// 画像が無い場合でもゲームは続行し、矩形描画を使う。
	bool Load();

	// 読み込んだ画像を解放する。
	void Release();

	// 現在のステージ情報に合わせてブロックを配置する。
	void Initialize(const StageData& currentStage);

	// すべてのブロックを更新する。
	void Update(
		const GravitySystem& gravitySystem,
		const StageData& currentStage
	);

	// カメラ範囲と重なるブロックへ重力反転を通知する。
	void OnGravityReversed(
		float cameraX,
		float cameraY,
		float cameraWidth,
		float cameraHeight
	);

	// すべてのブロックを描画する。
	void Draw() const;

	// 着地したブロックの位置にエフェクトを再生し、再生したかを返す。
	bool PlayLandingEffects(EffectManager& effectManager) const;

	// プレイヤーがブロックへ横方向から触れていたら押し戻す。
	bool ResolvePlayerHorizontalCollision(Player& player) const;

	// プレイヤーがブロックへ縦方向から触れていたら接地させる。
	bool ResolvePlayerVerticalCollision(
		Player& player,
		const GravitySystem& gravitySystem
	) const;

	// ダメージ状態のブロックがプレイヤーに当たっているか調べる。
	bool CheckPlayerCollision(
		const Player& player,
		float& damageSourceCenterX
	) const;

	// 敵弾が重力ブロックに当たっているか調べる。
	bool CheckEnemyBulletCollision(const EnemyBullet& bullet) const;

	// 通常弾が移動中に重力ブロックへ触れたか調べる。ブロックにはダメージを与えない。
	bool CheckPlayerBulletCollision(
		const Bullet& bullet, const Vector2& previousPosition) const;

	// 落下中のブロックと敵の当たり判定を行う。
	void CheckEnemyCollisions(
		EnemyManager& enemyManager,
		EffectManager& effectManager,
		ScoreManager& scoreManager
	) const;

private:
	static constexpr int MaxBlocks = StageData::MaxGravityBlockSpawns;

	// 登録済みのブロックをすべて消す。
	void Clear();

	// ブロックを1つ追加する。
	void AddBlock(
		const Vector2& position,
		GravityDirection gravityDirection,
		GravityBlockMotion motion
	);

	int imageHandle_ = -1;

	std::array<GravityBlock, MaxBlocks> blocks_{};
	int blockCount_ = 0;
};
