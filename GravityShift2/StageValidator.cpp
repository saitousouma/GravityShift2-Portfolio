#include "StageValidator.h"

#include <algorithm>
#include <initializer_list>

std::vector<StageValidationIssue> StageValidator::Validate(const StageData& stageData)
{
	std::vector<StageValidationIssue> issues;

	// クリア条件に必要な開始位置・ゴール・ボスが揃っているかを調べる。
	if (!stageData.hasPlayerStartPosition)
	{
		issues.push_back({ L"プレイヤー開始位置がありません" });
	}
	if (stageData.clearConditionType == ClearConditionType::ReachGoal &&
		!stageData.hasGoalPosition)
	{
		issues.push_back({ L"ゴールがありません" });
	}
	if (stageData.clearConditionType == ClearConditionType::DefeatMidBoss &&
		!stageData.hasMidBossSpawn)
	{
		issues.push_back({ L"中ボスがありません" });
	}
	if (stageData.clearConditionType == ClearConditionType::DefeatBoss &&
		!stageData.hasBossSpawn)
	{
		issues.push_back({ L"大ボスがありません" });
	}
	if (stageData.hasMidBossSpawn && stageData.hasBossSpawn)
	{
		issues.push_back({ L"中ボスと大ボスは同時に配置できません" });
	}

	// 高さが不正な場合は以降の配列参照が危険なため、ここで検証を終える。
	if (stageData.tileRowCount < 4 || stageData.tileRowCount > StageData::TileRows)
	{
		issues.push_back({ L"ステージの高さが無効です" });
		return issues;
	}

	// 可変配置オブジェクトの登録数が、それぞれの上限内かを調べる。
	if (stageData.fallHoleCount < 0 || stageData.fallHoleCount > StageData::MaxFallHoles)
	{
		issues.push_back({ L"穴の数が上限を超えています" });
	}
	if (stageData.gravityBlockSpawnCount < 0 ||
		stageData.gravityBlockSpawnCount > StageData::MaxGravityBlockSpawns)
	{
		issues.push_back({ L"重力ブロックの数が上限を超えています" });
	}
	if (stageData.enemySpawnCount < 0 ||
		stageData.enemySpawnCount > StageData::MaxEnemySpawns)
	{
		issues.push_back({ L"敵の数が上限を超えています" });
	}
	if (stageData.checkpointCount < 0 ||
		stageData.checkpointCount > StageData::MaxCheckpoints)
	{
		issues.push_back({ L"復活地点の数が上限を超えています" });
	}
	// 復活地点とノコギリがステージ内の空いている位置にあるかを調べる。
	const int checkpointCount = std::max(0, std::min(
		stageData.checkpointCount, StageData::MaxCheckpoints));
	for (int i = 0; i < checkpointCount; ++i)
	{
		const Vector2& position = stageData.checkpoints[i];
		const int column = static_cast<int>(position.x) / StageData::TileSize;
		const int row = static_cast<int>(position.y) / StageData::TileSize;
		if (position.x < 0.0f || position.x >= StageData::StagePixelWidth ||
			position.y < 0.0f || position.y >= stageData.GetPixelHeight() ||
			StageData::IsSolidTile(stageData.tiles[row][column]))
		{
			issues.push_back({ L"復活地点の位置が無効です", column, row });
		}
	}
	const int sawCount = std::max(0, std::min(stageData.sawSpawnCount,
		static_cast<int>(stageData.sawSpawns.size())));
	for (int i = 0; i < sawCount; ++i)
	{
		const Vector2& position = stageData.sawSpawns[i].position;
		const int column = static_cast<int>(position.x) / StageData::TileSize;
		const int row = static_cast<int>(position.y) / StageData::TileSize;
		if (position.x < 0.0f || position.x >= StageData::StagePixelWidth ||
			position.y < 0.0f || position.y >= stageData.GetPixelHeight() ||
			StageData::IsSolidTile(stageData.tiles[row][column]))
		{
			issues.push_back({ L"ノコギリの位置が無効です", column, row });
		}
	}
	// カメラ範囲がステージ内で正しい矩形になっているかを調べる。
	if (stageData.cameraAreaCount < 0 ||
		stageData.cameraAreaCount > StageData::MaxCameraAreas)
	{
		issues.push_back({ L"カメラ範囲の数が上限を超えています" });
	}
	const int areaCount = std::max(0, std::min(stageData.cameraAreaCount,
		StageData::MaxCameraAreas));
	for (int i = 0; i < areaCount; ++i)
	{
		const CameraArea& area = stageData.cameraAreas[i];
		if (area.left < 0 || area.top < 0 ||
			area.right > StageData::StagePixelWidth ||
			area.bottom > stageData.GetPixelHeight() ||
			area.left >= area.right || area.top >= area.bottom)
		{
			issues.push_back({ L"カメラ範囲が無効です",
				area.left / StageData::TileSize,
				area.top / StageData::TileSize });
		}
	}
	// ボスのHPと、戦闘に使うカメラグループの対応を調べる。
	if (stageData.hasMidBossSpawn)
	{
		if (stageData.midBossSpawn.hitPoint < 1 ||
			stageData.midBossSpawn.hitPoint > 99)
		{
			issues.push_back({ L"中ボスのHPは1～99にしてください",
				static_cast<int>(stageData.midBossSpawn.position.x) /
					StageData::TileSize,
				static_cast<int>(stageData.midBossSpawn.position.y) /
					StageData::TileSize });
		}
		bool hasMatchingCameraGroup = false;
		for (int i = 0; i < areaCount; ++i)
		{
			const int groupId = stageData.cameraAreas[i].groupId >= 0
				? stageData.cameraAreas[i].groupId : i;
			if (groupId == stageData.midBossSpawn.cameraGroupId)
			{
				hasMatchingCameraGroup = true;
				break;
			}
		}
		if (!hasMatchingCameraGroup)
		{
			issues.push_back({ L"中ボス用のカメラグループがありません",
				static_cast<int>(stageData.midBossSpawn.position.x) /
					StageData::TileSize,
				static_cast<int>(stageData.midBossSpawn.position.y) /
					StageData::TileSize });
		}
	}
	if (stageData.hasBossSpawn)
	{
		if (stageData.bossSpawn.hitPoint < 1 ||
			stageData.bossSpawn.hitPoint > 99)
		{
			issues.push_back({ L"大ボスのHPは1～99にしてください",
				static_cast<int>(stageData.bossSpawn.position.x) /
					StageData::TileSize,
				static_cast<int>(stageData.bossSpawn.position.y) /
					StageData::TileSize });
		}
		bool hasMatchingCameraGroup = false;
		for (int i = 0; i < areaCount; ++i)
		{
			const int groupId = stageData.cameraAreas[i].groupId >= 0
				? stageData.cameraAreas[i].groupId : i;
			if (groupId == stageData.bossSpawn.cameraGroupId)
			{
				hasMatchingCameraGroup = true;
				break;
			}
		}
		if (!hasMatchingCameraGroup)
		{
			issues.push_back({ L"大ボス用のカメラグループがありません",
				static_cast<int>(stageData.bossSpawn.position.x) /
					StageData::TileSize,
				static_cast<int>(stageData.bossSpawn.position.y) /
					StageData::TileSize });
		}
	}

	// 穴の境界、重複、内部に残った地形をまとめて検証する。
	const int holeCount = std::max(0, std::min(stageData.fallHoleCount,
		StageData::MaxFallHoles));
	for (int i = 0; i < holeCount; ++i)
	{
		const FallHole& hole = stageData.fallHoles[i];
		const bool isLegacy = hole.IsLegacyBoundaryHole();
		if (hole.left < 0 || hole.right > StageData::StagePixelWidth ||
			hole.left >= hole.right ||
			hole.left % StageData::TileSize != 0 ||
			hole.right % StageData::TileSize != 0 ||
			(!isLegacy && (hole.top < 0 ||
				hole.bottom > stageData.GetPixelHeight() ||
				hole.top >= hole.bottom ||
				hole.top % StageData::TileSize != 0 ||
				hole.bottom % StageData::TileSize != 0)))
		{
			issues.push_back({ L"穴の範囲が無効です" });
			continue;
		}

		for (int previous = 0; previous < i; ++previous)
		{
			const FallHole& other = stageData.fallHoles[previous];
			const bool overlapsVertically = isLegacy ||
				other.IsLegacyBoundaryHole() ||
				(hole.top < other.bottom && other.top < hole.bottom);
			if (overlapsVertically &&
				hole.left < other.right && other.left < hole.right)
			{
				issues.push_back({ L"穴の範囲が重なっています",
					hole.left / StageData::TileSize, 0 });
				break;
			}
		}

		// 穴の矩形内には地形があってはならない。
		for (int column = hole.left / StageData::TileSize;
			column < hole.right / StageData::TileSize; ++column)
		{
			for (int row = 0; row < stageData.tileRowCount; ++row)
			{
				const int tileTop = row * StageData::TileSize;
				const bool isHoleRow = isLegacy
					? row < 2 || row >= stageData.tileRowCount - 2
					: tileTop < hole.bottom &&
						tileTop + StageData::TileSize > hole.top;
				if (!isHoleRow)
				{
					continue;
				}
				if (StageData::IsSolidTile(stageData.tiles[row][column]))
				{
					issues.push_back({ L"穴の中に地形があります", column, row });
				}
			}
		}
	}

	// 配置済みの敵に有効なHPが設定されているかを調べる。
	const int enemyCount = std::max(0, std::min(stageData.enemySpawnCount,
		StageData::MaxEnemySpawns));
	for (int i = 0; i < enemyCount; ++i)
	{
		if (stageData.enemySpawns[i].hitPoint <= 0)
		{
			issues.push_back({ L"敵のHPは1以上にしてください",
				static_cast<int>(stageData.enemySpawns[i].position.x) / StageData::TileSize,
				static_cast<int>(stageData.enemySpawns[i].position.y) / StageData::TileSize });
		}
	}

	return issues;
}
