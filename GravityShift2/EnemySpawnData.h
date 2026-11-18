#pragma once

#include "Vector2.h"
#include "EnemyType.h"

// 敵を床に出すか天井に出すかを指定する列挙型
enum class EnemySpawnSurface
{
	Floor,    // 床
	Ceiling,  // 天井
};
enum class EnemySpawnFacing
{
	Left,
	Right,
};


// 敵一体のスポーンデータを保持する構造体
struct EnemySpawnData
{
	Vector2 position{};  // 敵のスポーン位置
	int hitPoint = 1;  // 敵のヒットポイント
	EnemySpawnSurface surface =
		EnemySpawnSurface::Floor;
	EnemyType type = EnemyType::Normal;  // 敵の種類
	// 既存ステージの挙動を保つため、初期値では重力反転の影響を受ける。
	bool isAffectedByGravityReversal = true;
	EnemySpawnFacing facing = EnemySpawnFacing::Left;
};
