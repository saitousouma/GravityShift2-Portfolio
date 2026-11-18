#include "StagePreviewSpawn.h"

#include <cmath>
#include "SawHazard.h"

bool StagePreviewSpawn::TryGetPosition(const StageData& stageData,
	int column, int row, float playerWidth, float playerHeight,
	Vector2& position)
{
	// マス番号とプレイヤーの大きさが、配置可能な範囲にあるかを確認する。
	if (stageData.tileRowCount < 1 || stageData.tileRowCount > StageData::TileRows ||
		column < 0 || column >= StageData::TileColumns ||
		row < 0 || row >= stageData.tileRowCount ||
		playerWidth <= 0.0f || playerHeight <= 0.0f)
	{
		return false;
	}

	const float left = column * StageData::TileSize +
		(StageData::TileSize - playerWidth) * 0.5f;
	const float top = (row + 1) * StageData::TileSize - playerHeight;
	const float right = left + playerWidth;
	const float bottom = top + playerHeight;
	if (left < 0.0f || top < 0.0f ||
		right > StageData::StagePixelWidth ||
		bottom > stageData.GetPixelHeight())
	{
		return false;
	}

	// 体がまたがる全マスを調べ、床や壁に埋まる位置を除外する。
	const int firstColumn = static_cast<int>(left / StageData::TileSize);
	const int lastColumn =
		static_cast<int>(std::ceil(right / StageData::TileSize)) - 1;
	const int firstRow = static_cast<int>(top / StageData::TileSize);
	const int lastRow =
		static_cast<int>(std::ceil(bottom / StageData::TileSize)) - 1;
	for (int tileRow = firstRow; tileRow <= lastRow; ++tileRow)
	{
		for (int tileColumn = firstColumn; tileColumn <= lastColumn; ++tileColumn)
		{
			if (StageData::IsSolidTile(stageData.tiles[tileRow][tileColumn]))
			{
				return false;
			}
		}
	}

	// 開始直後にノコギリへ触れる位置も配置候補から除外する。
	for (int i = 0; i < stageData.sawSpawnCount; ++i)
	{
		if (SawHazard::IntersectsRect(stageData.sawSpawns[i].position,
			{ left, top }, playerWidth, playerHeight))
		{
			return false;
		}
	}

	position = { left, top };
	return true;
}
