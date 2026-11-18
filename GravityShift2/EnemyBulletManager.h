#pragma once

#include "EnemyBullet.h"
#include <array>

class Player;
class GravityBlockManager;
struct StageData;

// 敵弾をまとめて管理するクラス。
// 生成、更新、描画、プレイヤーとの当たり判定を担当する。
class EnemyBulletManager
{
public:
	// カメラ変換に追従して描画する敵弾画像を読み込む。
	bool Load();

	// 読み込んだ敵弾画像を解放する。
	void Release();

	// 指定位置からプレイヤー方向へ弾を撃つ。
	void Shoot(const Vector2& startPosition, const Vector2& targetPosition);
	// 射撃敵の頭上方向へ、穴をループする縦弾を撃つ。
	void ShootVerticalLoop(const Vector2& startPosition, bool shootDown);
	// 固定砲台敵から、プレイヤーの重力方向へ縦弾を撃つ。
	void ShootMortar(const Vector2& startPosition, bool shootDown);

	// 全ての敵弾を更新する。
	void Update(
		const StageData& currentStage,
		const GravityBlockManager& gravityBlockManager
	);

	// 全ての敵弾を描画する。
	void Draw() const;

	// ステージ切り替えやリトライ時に、残っている敵弾を消す。
	void Clear();

	// プレイヤーに敵弾が当たっているか調べ、当たった弾は消す。
	bool CheckPlayerCollision(
		const Player& player,
		float& damageSourceCenterX
	);

private:
	// 空いている弾スロットを探して、新しい弾を生成する。
	bool SpawnBullet(const Vector2& centerPosition, const Vector2& velocity,
		EnemyBulletType type);
	// 縦弾の着弾位置から左右へ一発ずつ分裂弾を出す。
	void SpawnMortarSplit(const Vector2& centerPosition);
	// ループ穴に入った縦弾を、対応する出口へ移動する。
	bool TryLoopVerticalBullet(EnemyBullet& bullet,
		const StageData& currentStage) const;
	// 敵弾が床・天井に当たっているか調べる。
	bool IsCollidingWithStage(
		const EnemyBullet& bullet,
		const Vector2& previousPosition,
		const StageData& currentStage
	) const;

	static constexpr int MaxEnemyBullets = 32;
	static constexpr float BulletSpeed = 10.0f;
	static constexpr float MortarVerticalSpeed = 8.0f;
	static constexpr float MortarHorizontalSpeed = 7.0f;

	std::array<EnemyBullet, MaxEnemyBullets> bullets_{};
	int bulletImageHandle_ = -1;
	int mortarBulletImageHandle_ = -1;
};
