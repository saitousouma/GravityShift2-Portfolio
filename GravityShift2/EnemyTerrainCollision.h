#pragma once

#include "Vector2.h"

struct StageData;

// 敵の横移動を地形タイルの壁で止めるための当たり判定。
class EnemyTerrainCollision
{
public:
	// 壁に当たった場合はtrueを返し、停止させるX座標をresolvedXへ格納する。
	static bool ResolveHorizontal(const StageData& stage,
		const Vector2& previousPosition, const Vector2& currentPosition,
		float enemyWidth, float enemyHeight, float& resolvedX);
};
