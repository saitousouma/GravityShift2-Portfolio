#pragma once

#include "Vector2.h"

struct StageData;

// 通常弾が移動中に地形へ触れたかを調べる。レーザーには使用しない。
class BulletTerrainCollision
{
public:
	// 移動中の通常弾が指定した矩形に触れたかを調べる。
	static bool IntersectsRect(const Vector2& previousPosition,
		const Vector2& currentPosition, float bulletWidth,
		float bulletHeight, const Vector2& rectPosition,
		float rectWidth, float rectHeight);

	// 移動経路上に、穴ではない地形があるかを調べる。
	static bool IsBlocked(const StageData& stage,
		const Vector2& previousPosition, const Vector2& currentPosition,
		float bulletWidth, float bulletHeight);
};
