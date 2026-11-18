#pragma once

#include "EnemySpawnData.h"
#include "Vector2.h"

// ステージエディターから保存される中ボスの配置設定。
struct MidBossSpawnData
{
	static constexpr int Size = 128;
	static constexpr int DefaultHitPoint = 10;

	Vector2 position{}; // 描画と当たり判定に使う左上座標。
	int hitPoint = DefaultHitPoint; // 戦闘開始時のHP。
	EnemySpawnSurface surface = EnemySpawnSurface::Floor; // 初期の接地面。
	EnemySpawnFacing facing = EnemySpawnFacing::Left; // 初期の向き。
	int cameraGroupId = -1; // ボス戦を開始するカメラグループ。
};
