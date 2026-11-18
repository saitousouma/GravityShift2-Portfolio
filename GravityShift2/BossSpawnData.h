#pragma once

#include "EnemySpawnData.h"
#include "Vector2.h"

// ステージエディターから保存される大ボスの配置設定。
struct BossSpawnData
{
	static constexpr int Width = 180;
	static constexpr int Height = 120;
	static constexpr int DefaultHitPoint = 20;

	Vector2 position{}; // 描画と当たり判定に使う左上座標。
	int hitPoint = DefaultHitPoint; // 戦闘開始時のHP。
	EnemySpawnFacing facing = EnemySpawnFacing::Left; // 初期の向き。
	int cameraGroupId = -1; // ボス戦を開始するカメラグループ。
};
