#include "EnemyTerrainCollision.h"

#include <algorithm>

#include "StageData.h"

namespace
{
	// 穴と重なるタイルは、敵が通過できる地形として扱う。
	bool IsHoleTile(const StageData& stage, int tileLeft, int tileRight,
		int tileTop, int tileBottom)
	{
		for (int i = 0; i < stage.fallHoleCount; ++i)
		{
			if (stage.fallHoles[i].OverlapsTile(tileLeft, tileRight,
				tileTop, tileBottom, stage.GetPixelHeight(),
				StageData::TileSize * 2))
			{
				return true;
			}
		}

		return false;
	}
}

bool EnemyTerrainCollision::ResolveHorizontal(const StageData& stage,
	const Vector2& previousPosition, const Vector2& currentPosition,
	float enemyWidth, float enemyHeight, float& resolvedX)
{
	// 横移動がない場合や、当たり判定の大きさが不正な場合は処理しない。
	const float movementX = currentPosition.x - previousPosition.x;
	if (movementX == 0.0f || enemyWidth <= 0.0f || enemyHeight <= 0.0f)
	{
		return false;
	}

	const bool movingRight = movementX > 0.0f;
	const float previousLeft = previousPosition.x;
	const float previousRight = previousLeft + enemyWidth;
	const float currentLeft = currentPosition.x;
	const float currentRight = currentLeft + enemyWidth;
	const float verticalInset = std::min(2.0f, enemyHeight * 0.1f);
	const float bodyTop = currentPosition.y + verticalInset;
	const float bodyBottom = currentPosition.y + enemyHeight - verticalInset;

	// 敵の移動前後を含む範囲から、調べるタイルを絞り込む。
	const int rowCount = std::min(stage.tileRowCount,
		static_cast<int>(stage.tiles.size()));
	const int firstRow = std::max(0,
		static_cast<int>(bodyTop) / StageData::TileSize);
	const int lastRow = std::min(rowCount - 1,
		static_cast<int>(bodyBottom - 0.001f) / StageData::TileSize);
	const float sweptLeft = std::min(previousLeft, currentLeft);
	const float sweptRight = std::max(previousRight, currentRight);
	const int firstColumn = std::max(0,
		static_cast<int>(sweptLeft) / StageData::TileSize);
	const int lastColumn = std::min(StageData::TileColumns - 1,
		static_cast<int>(sweptRight - 0.001f) / StageData::TileSize);

	if (firstRow > lastRow || firstColumn > lastColumn)
	{
		return false;
	}

	bool collided = false;
	float collisionBoundary = movingRight
		? static_cast<float>(StageData::StagePixelWidth)
		: 0.0f;

	// 最も手前にある壁の境界を記録し、敵をその直前へ戻す。
	for (int row = firstRow; row <= lastRow; ++row)
	{
		const int tileTop = row * StageData::TileSize;
		const int tileBottom = tileTop + StageData::TileSize;
		if (bodyTop >= static_cast<float>(tileBottom) ||
			bodyBottom <= static_cast<float>(tileTop))
		{
			continue;
		}

		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (!StageData::IsSolidTile(stage.tiles[row][column]))
			{
				continue;
			}

			const int tileLeft = column * StageData::TileSize;
			const int tileRight = tileLeft + StageData::TileSize;
			if (IsHoleTile(stage, tileLeft, tileRight, tileTop, tileBottom))
			{
				continue;
			}

			const bool overlapsAtCurrentPosition =
				currentLeft < static_cast<float>(tileRight) &&
				currentRight > static_cast<float>(tileLeft);
			const bool crossedWall = movingRight
				? previousRight <= static_cast<float>(tileLeft) &&
					currentRight >= static_cast<float>(tileLeft)
				: previousLeft >= static_cast<float>(tileRight) &&
					currentLeft <= static_cast<float>(tileRight);

			if (!overlapsAtCurrentPosition && !crossedWall)
			{
				continue;
			}

			collided = true;
			if (movingRight)
			{
				collisionBoundary = std::min(collisionBoundary,
					static_cast<float>(tileLeft));
			}
			else
			{
				collisionBoundary = std::max(collisionBoundary,
					static_cast<float>(tileRight));
			}
		}
	}

	if (!collided)
	{
		return false;
	}

	// 敵の外周が壁の境界に接する位置を解決後の座標にする。
	resolvedX = movingRight
		? collisionBoundary - enemyWidth
		: collisionBoundary;
	return true;
}
