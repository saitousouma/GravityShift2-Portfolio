#include "BulletTerrainCollision.h"

#include "StageData.h"

bool BulletTerrainCollision::IntersectsRect(
	const Vector2& previousPosition, const Vector2& currentPosition,
	float bulletWidth, float bulletHeight, const Vector2& rectPosition,
	float rectWidth, float rectHeight)
{
	// 移動前後の弾を囲む矩形を作り、高速な弾のすり抜けを防ぐ。
	const float bulletLeft = previousPosition.x < currentPosition.x
		? previousPosition.x : currentPosition.x;
	const float bulletRight = (previousPosition.x > currentPosition.x
		? previousPosition.x : currentPosition.x) + bulletWidth;
	const float bulletTop = previousPosition.y < currentPosition.y
		? previousPosition.y : currentPosition.y;
	const float bulletBottom = (previousPosition.y > currentPosition.y
		? previousPosition.y : currentPosition.y) + bulletHeight;
	return bulletLeft < rectPosition.x + rectWidth &&
		bulletRight > rectPosition.x &&
		bulletTop < rectPosition.y + rectHeight &&
		bulletBottom > rectPosition.y;
}

bool BulletTerrainCollision::IsBlocked(const StageData& stage,
	const Vector2& previousPosition, const Vector2& currentPosition,
	float bulletWidth, float bulletHeight)
{
	// 通常弾の横移動前後を囲む範囲で判定し、薄い地形も飛び越えない。
	const float bulletTop = previousPosition.y < currentPosition.y
		? previousPosition.y : currentPosition.y;
	const float bulletBottom = (previousPosition.y > currentPosition.y
		? previousPosition.y : currentPosition.y) + bulletHeight;

	// 弾と縦方向が重なる行だけを対象に、衝突するタイルを探す。
	for (int row = 0; row < stage.tileRowCount &&
		row < StageData::TileRows; ++row)
	{
		const int tileTop = row * StageData::TileSize;
		const int tileBottom = tileTop + StageData::TileSize;
		if (bulletTop >= tileBottom || bulletBottom <= tileTop)
		{
			continue;
		}
		for (int column = 0; column < StageData::TileColumns; ++column)
		{
			if (!StageData::IsSolidTile(stage.tiles[row][column]))
			{
				continue;
			}
			const int tileLeft = column * StageData::TileSize;
			const int tileRight = tileLeft + StageData::TileSize;
			if (!IntersectsRect(previousPosition, currentPosition,
				bulletWidth, bulletHeight,
				{ static_cast<float>(tileLeft), static_cast<float>(tileTop) },
				static_cast<float>(tileRight - tileLeft),
				static_cast<float>(tileBottom - tileTop)))
			{
				continue;
			}

			// 穴の範囲にあるタイルは通過可能なため、壁として扱わない。
			bool isHoleTile = false;
			for (int i = 0; i < stage.fallHoleCount; ++i)
			{
				const FallHole& hole = stage.fallHoles[i];
				if (hole.OverlapsTile(tileLeft, tileRight, tileTop, tileBottom,
					stage.GetPixelHeight(), StageData::TileSize * 2))
				{
					isHoleTile = true;
					break;
				}
			}
			if (!isHoleTile)
			{
				return true;
			}
		}
	}
	return false;
}
