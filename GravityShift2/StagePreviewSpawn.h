#pragma once

#include "StageData.h"

// 指定マスを足元として、プレイヤーが地形に重ならない開始位置を求める。
class StagePreviewSpawn
{
public:
	static bool TryGetPosition(const StageData& stageData,
		int column, int row, float playerWidth, float playerHeight,
		Vector2& position);
};
