#include "EditorStageData.h"

#include <algorithm>
#include <cmath>

namespace
{
	float GetEditorEnemySize(EnemyType type)
	{
		if (type == EnemyType::Heavy)
		{
			return 56.0f;
		}
		if (type == EnemyType::Charger)
		{
			return 80.0f;
		}
		if (type == EnemyType::Mortar)
		{
			return 60.0f;
		}
		return 48.0f;
	}
}

// 元データを値としてコピーするため、ここで変更してもゲーム本編には影響しない。
void EditorStageData::LoadFrom(const StageData& sourceStageData)
{
	stageData_ = sourceStageData;
	int nextGroupId = 0;
	for (int i = 0; i < stageData_.cameraAreaCount; ++i)
	{
		if (stageData_.cameraAreas[i].groupId >= nextGroupId)
		{
			nextGroupId = stageData_.cameraAreas[i].groupId + 1;
		}
	}
	for (int i = 0; i < stageData_.cameraAreaCount; ++i)
	{
		if (stageData_.cameraAreas[i].groupId < 0)
		{
			stageData_.cameraAreas[i].groupId = nextGroupId++;
		}
	}
}

bool EditorStageData::ExpandRows(int rowCount)
{
	if (rowCount <= 0 || stageData_.tileRowCount >= StageData::TileRows)
	{
		return false;
	}

	const int oldRowCount = stageData_.tileRowCount;
	stageData_.tileRowCount = (std::min)(
		StageData::TileRows,
		stageData_.tileRowCount + rowCount);
	for (int row = oldRowCount; row < stageData_.tileRowCount; ++row)
	{
		stageData_.tiles[row].fill(StageData::EmptyTile);
		stageData_.solidVisuals[row].fill(VisualTile{});
		stageData_.backgroundVisuals[row].fill(VisualTile{});
	}

	return true;
}

// 選択中の種類に応じて、地形または1つだけ存在できる特殊位置を変更する。
bool EditorStageData::Place(int column, int row, char item)
{
	if (!IsInsideStage(column, row))
	{
		return false;
	}

	const Vector2 position{
		static_cast<float>(column * StageData::TileSize),
		static_cast<float>(row * StageData::TileSize)
	};
	bool hasChanged = false;

	if (item == StageData::GravityBlockTile)
	{
		return PlaceGravityBlock(column, row);
	}
	if (item == StageData::CheckpointTile)
	{
		return PlaceCheckpoint(column, row);
	}

	if (item == StageData::NormalEnemyTile ||
		item == StageData::HeavyEnemyTile ||
		item == StageData::ShooterEnemyTile ||
		item == StageData::UpShooterEnemyTile ||
		item == StageData::ChargerEnemyTile ||
		item == StageData::MortarEnemyTile)
	{
		return PlaceEnemy(column, row, item);
	}

	if (item == StageData::PlayerStartTile)
	{
		hasChanged = !stageData_.hasPlayerStartPosition ||
			!IsPositionInCell(stageData_.playerStartPosition, column, row);
		stageData_.playerStartPosition = position;
		stageData_.hasPlayerStartPosition = true;

		// プレイヤーとゴールが同じマスにならないよう、後から置いた方を優先する。
		if (stageData_.hasGoalPosition &&
			IsPositionInCell(stageData_.goalPosition, column, row))
		{
			stageData_.hasGoalPosition = false;
			hasChanged = true;
		}

		hasChanged = RemoveGravityBlockAt(column, row) || hasChanged;
		hasChanged = RemoveEnemyAt(column, row) || hasChanged;
		hasChanged = RemoveSawAt(column, row) || hasChanged;
		hasChanged = RemoveCheckpointAt(column, row) || hasChanged;

		hasChanged = SetTile(column, row, StageData::EmptyTile) || hasChanged;
		return hasChanged;
	}

	if (item == StageData::GoalTile)
	{
		hasChanged = !stageData_.hasGoalPosition ||
			!IsPositionInCell(stageData_.goalPosition, column, row);
		stageData_.goalPosition = position;
		stageData_.hasGoalPosition = true;

		if (stageData_.hasPlayerStartPosition &&
			IsPositionInCell(stageData_.playerStartPosition, column, row))
		{
			stageData_.hasPlayerStartPosition = false;
			hasChanged = true;
		}

		hasChanged = RemoveGravityBlockAt(column, row) || hasChanged;
		hasChanged = RemoveEnemyAt(column, row) || hasChanged;
		hasChanged = RemoveSawAt(column, row) || hasChanged;
		hasChanged = RemoveCheckpointAt(column, row) || hasChanged;

		hasChanged = SetTile(column, row, StageData::EmptyTile) || hasChanged;
		return hasChanged;
	}

	// 地形を置くマスに特殊オブジェクトがあれば取り除く。
	if (stageData_.hasPlayerStartPosition &&
		IsPositionInCell(stageData_.playerStartPosition, column, row))
	{
		stageData_.hasPlayerStartPosition = false;
		hasChanged = true;
	}

	if (stageData_.hasGoalPosition &&
		IsPositionInCell(stageData_.goalPosition, column, row))
	{
		stageData_.hasGoalPosition = false;
		hasChanged = true;
	}

	hasChanged = RemoveGravityBlockAt(column, row) || hasChanged;
	hasChanged = RemoveEnemyAt(column, row) || hasChanged;
	hasChanged = RemoveSawAt(column, row) || hasChanged;
	hasChanged = RemoveCheckpointAt(column, row) || hasChanged;

	return SetTile(column, row, item) || hasChanged;
}

bool EditorStageData::FillRectangle(int firstColumn, int firstRow,
	int lastColumn, int lastRow, char tile)
{
	if (tile != StageData::FloorTile && tile != StageData::WallTile &&
		tile != StageData::BlockTile &&
		tile != StageData::InvisibleBlockTile)
	{
		return false;
	}

	const int left = (std::min)(firstColumn, lastColumn);
	const int right = (std::max)(firstColumn, lastColumn);
	const int top = (std::min)(firstRow, lastRow);
	const int bottom = (std::max)(firstRow, lastRow);
	if (!IsInsideStage(left, top) || !IsInsideStage(right, bottom))
	{
		return false;
	}

	bool changed = false;
	for (int row = top; row <= bottom; ++row)
	{
		for (int column = left; column <= right; ++column)
		{
			// 単独配置と同じルールで、範囲内の特殊オブジェクトも整理する。
			changed = Place(column, row, tile) || changed;
		}
	}
	return changed;
}

bool EditorStageData::PlaceVisual(int column, int row,
	const std::string& sheetName, int sourceColumn, int sourceRow, char role,
	int frameCount, bool flipX, bool flipY)
{
	if (!IsInsideStage(column, row) || sheetName.empty() ||
		sourceColumn < 0 || sourceColumn > 255 ||
		sourceRow < 0 || sourceRow > 255 ||
		frameCount < 1 || frameCount > 255 ||
		(role != StageData::FloorTile && role != StageData::WallTile &&
		role != StageData::BlockTile && role != StageData::EmptyTile))
	{
		return false;
	}

	int sheetIndex = -1;
	for (int i = 0; i < stageData_.tileSheetCount; ++i)
	{
		if (stageData_.tileSheetNames[i] == sheetName)
		{
			sheetIndex = i;
			break;
		}
	}
	if (sheetIndex < 0)
	{
		if (stageData_.tileSheetCount >= StageData::MaxTileSheets)
		{
			return false;
		}
		sheetIndex = stageData_.tileSheetCount++;
		stageData_.tileSheetNames[sheetIndex] = sheetName;
	}

	const VisualTile tile{ static_cast<short>(sheetIndex),
		static_cast<unsigned char>(sourceColumn),
		static_cast<unsigned char>(sourceRow),
		static_cast<unsigned char>(frameCount),
		flipX,
		flipY };
	VisualTile& target = role == StageData::EmptyTile
		? stageData_.backgroundVisuals[row][column]
		: stageData_.solidVisuals[row][column];
	if (role == StageData::EmptyTile)
	{
		const bool changed = target.sheetIndex != tile.sheetIndex ||
			target.column != tile.column || target.row != tile.row ||
			target.frameCount != tile.frameCount ||
			target.flipX != tile.flipX || target.flipY != tile.flipY;
		target = tile;
		return changed;
	}

	if (stageData_.tiles[row][column] == role &&
		target.sheetIndex == tile.sheetIndex &&
		target.column == tile.column && target.row == tile.row &&
		target.frameCount == tile.frameCount &&
		target.flipX == tile.flipX && target.flipY == tile.flipY)
	{
		return false;
	}
	Place(column, row, role);
	stageData_.solidVisuals[row][column] = tile;
	return true;
}

bool EditorStageData::FillVisualRectangle(int firstColumn, int firstRow,
	int lastColumn, int lastRow, const std::string& sheetName,
	int sourceColumn, int sourceRow, char role, int frameCount,
	bool flipX, bool flipY)
{
	const int left = (std::min)(firstColumn, lastColumn);
	const int right = (std::max)(firstColumn, lastColumn);
	const int top = (std::min)(firstRow, lastRow);
	const int bottom = (std::max)(firstRow, lastRow);
	if (!IsInsideStage(left, top) || !IsInsideStage(right, bottom))
	{
		return false;
	}
	bool changed = false;
	for (int row = top; row <= bottom; ++row)
	{
		for (int column = left; column <= right; ++column)
		{
			changed = PlaceVisual(column, row, sheetName,
				sourceColumn, sourceRow, role, frameCount,
				flipX, flipY) || changed;
		}
	}
	return changed;
}

// 右クリックしたマスに存在するものをまとめて空にする。
bool EditorStageData::Erase(int column, int row)
{
	if (!IsInsideStage(column, row))
	{
		return false;
	}

	bool hasChanged = SetTile(column, row, StageData::EmptyTile);
	if (stageData_.backgroundVisuals[row][column].sheetIndex >= 0)
	{
		stageData_.backgroundVisuals[row][column] = {};
		hasChanged = true;
	}

	if (stageData_.hasPlayerStartPosition &&
		IsPositionInCell(stageData_.playerStartPosition, column, row))
	{
		stageData_.hasPlayerStartPosition = false;
		hasChanged = true;
	}

	if (stageData_.hasGoalPosition &&
		IsPositionInCell(stageData_.goalPosition, column, row))
	{
		stageData_.hasGoalPosition = false;
		hasChanged = true;
	}

	hasChanged = RemoveGravityBlockAt(column, row) || hasChanged;
	hasChanged = RemoveEnemyAt(column, row) || hasChanged;
	hasChanged = RemoveSawAt(column, row) || hasChanged;
	hasChanged = RemoveCheckpointAt(column, row) || hasChanged;
	hasChanged = RemoveMidBossAt(column, row) || hasChanged;
	hasChanged = RemoveBossAt(column, row) || hasChanged;

	return hasChanged;
}

bool EditorStageData::EraseRectangle(int firstColumn, int firstRow,
	int lastColumn, int lastRow)
{
	const int left = (std::min)(firstColumn, lastColumn);
	const int right = (std::max)(firstColumn, lastColumn);
	const int top = (std::min)(firstRow, lastRow);
	const int bottom = (std::max)(firstRow, lastRow);
	if (!IsInsideStage(left, top) || !IsInsideStage(right, bottom))
	{
		return false;
	}

	bool changed = false;
	for (int row = top; row <= bottom; ++row)
	{
		for (int column = left; column <= right; ++column)
		{
			changed = Erase(column, row) || changed;
		}
	}
	return changed;
}

const StageData& EditorStageData::GetStageData() const
{
	return stageData_;
}

int EditorStageData::FindCameraAreaAt(int column, int row) const
{
	const int worldX = column * StageData::TileSize + StageData::TileSize / 2;
	const int worldY = row * StageData::TileSize + StageData::TileSize / 2;
	for (int i = stageData_.cameraAreaCount - 1; i >= 0; --i)
	{
		if (stageData_.cameraAreas[i].Contains(
			static_cast<float>(worldX), static_cast<float>(worldY)))
		{
			return i;
		}
	}
	return -1;
}

bool EditorStageData::AddCameraArea(
	int firstColumn, int firstRow, int lastColumn, int lastRow)
{
	if (stageData_.cameraAreaCount >= StageData::MaxCameraAreas)
	{
		return false;
	}

	const int index = stageData_.cameraAreaCount;
	++stageData_.cameraAreaCount;
	if (!ReplaceCameraArea(index, firstColumn, firstRow,
		lastColumn, lastRow))
	{
		--stageData_.cameraAreaCount;
		return false;
	}
	stageData_.cameraAreas[index].mode = CameraAreaMode::Free;
	stageData_.cameraAreas[index].groupId = GetNextCameraAreaGroupId();
	return true;
}

bool EditorStageData::AddCameraAreaPart(
	int sourceIndex,
	int firstColumn,
	int firstRow,
	int lastColumn,
	int lastRow)
{
	if (sourceIndex < 0 || sourceIndex >= stageData_.cameraAreaCount ||
		stageData_.cameraAreaCount >= StageData::MaxCameraAreas)
	{
		return false;
	}

	const CameraArea source = stageData_.cameraAreas[sourceIndex];
	const int index = stageData_.cameraAreaCount++;
	if (!ReplaceCameraArea(index, firstColumn, firstRow, lastColumn, lastRow))
	{
		--stageData_.cameraAreaCount;
		return false;
	}
	stageData_.cameraAreas[index].mode = source.mode;
	stageData_.cameraAreas[index].groupId = source.groupId >= 0
		? source.groupId : GetNextCameraAreaGroupId();
	if (source.groupId < 0)
	{
		stageData_.cameraAreas[sourceIndex].groupId =
			stageData_.cameraAreas[index].groupId;
	}
	return true;
}

bool EditorStageData::ReplaceCameraArea(
	int index, int firstColumn, int firstRow, int lastColumn, int lastRow)
{
	if (index < 0 || index >= stageData_.cameraAreaCount ||
		index >= StageData::MaxCameraAreas ||
		!IsInsideStage(firstColumn, firstRow) ||
		!IsInsideStage(lastColumn, lastRow))
	{
		return false;
	}

	const int left = std::min(firstColumn, lastColumn) * StageData::TileSize;
	const int right = (std::max(firstColumn, lastColumn) + 1) * StageData::TileSize;
	const int top = std::min(firstRow, lastRow) * StageData::TileSize;
	const int bottom = (std::max(firstRow, lastRow) + 1) * StageData::TileSize;
	CameraArea& area = stageData_.cameraAreas[index];
	if (area.left == left && area.top == top &&
		area.right == right && area.bottom == bottom)
	{
		return false;
	}
	area.left = left;
	area.top = top;
	area.right = right;
	area.bottom = bottom;
	return true;
}

bool EditorStageData::CycleCameraAreaMode(int index)
{
	if (index < 0 || index >= stageData_.cameraAreaCount)
	{
		return false;
	}
	const int groupId = stageData_.cameraAreas[index].groupId;
	CameraAreaMode mode = stageData_.cameraAreas[index].mode;
	switch (mode)
	{
	case CameraAreaMode::Free: mode = CameraAreaMode::Horizontal; break;
	case CameraAreaMode::Horizontal: mode = CameraAreaMode::Vertical; break;
	case CameraAreaMode::Vertical: mode = CameraAreaMode::Locked; break;
	default: mode = CameraAreaMode::Free; break;
	}
	for (int i = 0; i < stageData_.cameraAreaCount; ++i)
	{
		if (i == index || (groupId >= 0 &&
			stageData_.cameraAreas[i].groupId == groupId))
		{
			stageData_.cameraAreas[i].mode = mode;
		}
	}
	return true;
}

bool EditorStageData::RemoveCameraArea(int index)
{
	if (index < 0 || index >= stageData_.cameraAreaCount)
	{
		return false;
	}
	for (int i = index; i + 1 < stageData_.cameraAreaCount; ++i)
	{
		stageData_.cameraAreas[i] = stageData_.cameraAreas[i + 1];
	}
	--stageData_.cameraAreaCount;
	stageData_.cameraAreas[stageData_.cameraAreaCount] = {};
	return true;
}

int EditorStageData::GetNextCameraAreaGroupId() const
{
	int nextGroupId = 0;
	for (int i = 0; i < stageData_.cameraAreaCount; ++i)
	{
		if (stageData_.cameraAreas[i].groupId >= nextGroupId)
		{
			nextGroupId = stageData_.cameraAreas[i].groupId + 1;
		}
	}
	return nextGroupId;
}

bool EditorStageData::HasEnemyAt(int column, int row) const
{
	return FindEnemyAt(column, row) >= 0;
}

bool EditorStageData::HasGravityBlockAt(int column, int row) const
{
	return FindGravityBlockAt(column, row) >= 0;
}

int EditorStageData::FindSawAt(int column, int row) const
{
	for (int i = 0; i < stageData_.sawSpawnCount; ++i)
	{
		if (IsPositionInCell(stageData_.sawSpawns[i].position, column, row))
		{
			return i;
		}
	}
	return -1;
}

bool EditorStageData::HasSawAt(int column, int row) const
{
	return FindSawAt(column, row) >= 0;
}

int EditorStageData::FindCheckpointAt(int column, int row) const
{
	for (int i = 0; i < stageData_.checkpointCount; ++i)
	{
		if (IsPositionInCell(stageData_.checkpoints[i], column, row))
		{
			return i;
		}
	}
	return -1;
}

bool EditorStageData::HasCheckpointAt(int column, int row) const
{
	return FindCheckpointAt(column, row) >= 0;
}

bool EditorStageData::HasMidBossAt(int column, int row) const
{
	if (!stageData_.hasMidBossSpawn)
	{
		return false;
	}

	constexpr int bossSize = MidBossSpawnData::Size;
	const MidBossSpawnData& spawn = stageData_.midBossSpawn;
	const int bossColumn =
		(static_cast<int>(spawn.position.x) + bossSize / 2) /
		StageData::TileSize;
	const int bossRow = spawn.surface == EnemySpawnSurface::Ceiling
		? static_cast<int>(spawn.position.y) / StageData::TileSize
		: (static_cast<int>(spawn.position.y) + bossSize - 1) /
			StageData::TileSize;
	return bossColumn == column && bossRow == row;
}

bool EditorStageData::RemoveMidBossAt(int column, int row)
{
	if (!HasMidBossAt(column, row))
	{
		return false;
	}
	stageData_.midBossSpawn = {};
	stageData_.hasMidBossSpawn = false;
	return true;
}

bool EditorStageData::PlaceMidBoss(int column, int row)
{
	const int areaIndex = FindCameraAreaAt(column, row);
	if (stageData_.hasBossSpawn || areaIndex < 0 ||
		!CanMoveObjectTo(column, row,
			static_cast<float>(MidBossSpawnData::Size),
			static_cast<float>(MidBossSpawnData::Size), -1, -1, true))
	{
		return false;
	}

	const bool isCeiling = IsCeilingPlacement(column, row);
	const CameraArea& area = stageData_.cameraAreas[areaIndex];
	MidBossSpawnData spawn{};
	spawn.position.x = column * StageData::TileSize +
		(StageData::TileSize - MidBossSpawnData::Size) * 0.5f;
	spawn.position.y = isCeiling
		? static_cast<float>(row * StageData::TileSize)
		: static_cast<float>((row + 1) * StageData::TileSize) -
			MidBossSpawnData::Size;
	spawn.surface = isCeiling
		? EnemySpawnSurface::Ceiling : EnemySpawnSurface::Floor;
	spawn.hitPoint = stageData_.hasMidBossSpawn
		? stageData_.midBossSpawn.hitPoint
		: MidBossSpawnData::DefaultHitPoint;
	spawn.facing = stageData_.hasMidBossSpawn
		? stageData_.midBossSpawn.facing : EnemySpawnFacing::Left;
	spawn.cameraGroupId = area.groupId >= 0 ? area.groupId : areaIndex;

	const bool unchanged = stageData_.hasMidBossSpawn &&
		stageData_.midBossSpawn.position.x == spawn.position.x &&
		stageData_.midBossSpawn.position.y == spawn.position.y &&
		stageData_.midBossSpawn.surface == spawn.surface &&
		stageData_.midBossSpawn.cameraGroupId == spawn.cameraGroupId;
	stageData_.midBossSpawn = spawn;
	stageData_.hasMidBossSpawn = true;
	return !unchanged;
}

bool EditorStageData::MoveMidBoss(int fromColumn, int fromRow,
	int toColumn, int toRow)
{
	if (!HasMidBossAt(fromColumn, fromRow) ||
		(fromColumn == toColumn && fromRow == toRow))
	{
		return false;
	}
	return PlaceMidBoss(toColumn, toRow);
}

bool EditorStageData::ToggleMidBossFacingAt(int column, int row)
{
	if (!HasMidBossAt(column, row))
	{
		return false;
	}
	MidBossSpawnData& spawn = stageData_.midBossSpawn;
	spawn.facing = spawn.facing == EnemySpawnFacing::Left
		? EnemySpawnFacing::Right : EnemySpawnFacing::Left;
	return true;
}

bool EditorStageData::HasBossAt(int column, int row) const
{
	if (!stageData_.hasBossSpawn)
	{
		return false;
	}

	const BossSpawnData& spawn = stageData_.bossSpawn;
	const int bossColumn =
		(static_cast<int>(spawn.position.x) + BossSpawnData::Width / 2) /
		StageData::TileSize;
	const int bossRow =
		(static_cast<int>(spawn.position.y) + BossSpawnData::Height - 1) /
		StageData::TileSize;
	return bossColumn == column && bossRow == row;
}

bool EditorStageData::RemoveBossAt(int column, int row)
{
	if (!HasBossAt(column, row))
	{
		return false;
	}
	stageData_.bossSpawn = {};
	stageData_.hasBossSpawn = false;
	return true;
}

bool EditorStageData::PlaceBoss(int column, int row)
{
	const int areaIndex = FindCameraAreaAt(column, row);
	if (stageData_.hasMidBossSpawn || areaIndex < 0 ||
		!CanMoveObjectTo(column, row,
			static_cast<float>(BossSpawnData::Width),
			static_cast<float>(BossSpawnData::Height),
			-1, -1, false, true, true))
	{
		return false;
	}

	const CameraArea& area = stageData_.cameraAreas[areaIndex];
	BossSpawnData spawn{};
	spawn.position.x = column * StageData::TileSize +
		(StageData::TileSize - BossSpawnData::Width) * 0.5f;
	spawn.position.y = static_cast<float>((row + 1) * StageData::TileSize) -
		BossSpawnData::Height;
	spawn.hitPoint = stageData_.hasBossSpawn
		? stageData_.bossSpawn.hitPoint
		: BossSpawnData::DefaultHitPoint;
	spawn.facing = stageData_.hasBossSpawn
		? stageData_.bossSpawn.facing : EnemySpawnFacing::Left;
	spawn.cameraGroupId = area.groupId >= 0 ? area.groupId : areaIndex;

	const bool unchanged = stageData_.hasBossSpawn &&
		stageData_.bossSpawn.position.x == spawn.position.x &&
		stageData_.bossSpawn.position.y == spawn.position.y &&
		stageData_.bossSpawn.cameraGroupId == spawn.cameraGroupId;
	stageData_.bossSpawn = spawn;
	stageData_.hasBossSpawn = true;
	return !unchanged;
}

bool EditorStageData::MoveBoss(int fromColumn, int fromRow,
	int toColumn, int toRow)
{
	if (!HasBossAt(fromColumn, fromRow) ||
		(fromColumn == toColumn && fromRow == toRow))
	{
		return false;
	}
	return PlaceBoss(toColumn, toRow);
}

bool EditorStageData::ToggleBossFacingAt(int column, int row)
{
	if (!HasBossAt(column, row))
	{
		return false;
	}
	BossSpawnData& spawn = stageData_.bossSpawn;
	spawn.facing = spawn.facing == EnemySpawnFacing::Left
		? EnemySpawnFacing::Right : EnemySpawnFacing::Left;
	return true;
}

bool EditorStageData::RemoveCheckpointAt(int column, int row)
{
	const int index = FindCheckpointAt(column, row);
	if (index < 0)
	{
		return false;
	}
	for (int i = index; i + 1 < stageData_.checkpointCount; ++i)
	{
		stageData_.checkpoints[i] = stageData_.checkpoints[i + 1];
	}
	stageData_.checkpoints[--stageData_.checkpointCount] = {};
	return true;
}

bool EditorStageData::PlaceCheckpoint(int column, int row)
{
	if (!IsInsideStage(column, row) || HasCheckpointAt(column, row) ||
		stageData_.checkpointCount >= StageData::MaxCheckpoints)
	{
		return false;
	}

	bool changed = SetTile(column, row, StageData::EmptyTile);
	if (stageData_.hasPlayerStartPosition &&
		IsPositionInCell(stageData_.playerStartPosition, column, row))
	{
		stageData_.hasPlayerStartPosition = false;
		changed = true;
	}
	if (stageData_.hasGoalPosition &&
		IsPositionInCell(stageData_.goalPosition, column, row))
	{
		stageData_.hasGoalPosition = false;
		changed = true;
	}
	changed = RemoveGravityBlockAt(column, row) || changed;
	changed = RemoveEnemyAt(column, row) || changed;
	changed = RemoveSawAt(column, row) || changed;

	stageData_.checkpoints[stageData_.checkpointCount++] = {
		static_cast<float>(column * StageData::TileSize),
		static_cast<float>(row * StageData::TileSize)
	};
	return true;
}

const SawSpawnData* EditorStageData::GetSawAt(int column, int row) const
{
	const int index = FindSawAt(column, row);
	return index >= 0 ? &stageData_.sawSpawns[index] : nullptr;
}

bool EditorStageData::FlipSawAt(int column, int row, bool horizontal)
{
	const int index = FindSawAt(column, row);
	if (index < 0)
	{
		return false;
	}
	SawSpawnData& saw = stageData_.sawSpawns[index];
	if (horizontal)
	{
		saw.flipX = !saw.flipX;
	}
	else
	{
		saw.flipY = !saw.flipY;
	}
	return true;
}

bool EditorStageData::RemoveSawAt(int column, int row)
{
	const int index = FindSawAt(column, row);
	if (index < 0)
	{
		return false;
	}
	stageData_.sawSpawns.erase(stageData_.sawSpawns.begin() + index);
	stageData_.sawSpawnCount =
		static_cast<int>(stageData_.sawSpawns.size());
	return true;
}

bool EditorStageData::PlaceSaw(int column, int row)
{
	if (!IsInsideStage(column, row) || HasSawAt(column, row) ||
		StageData::IsSolidTile(stageData_.tiles[row][column]) ||
		HasEnemyAt(column, row) || HasGravityBlockAt(column, row) ||
		HasCheckpointAt(column, row) ||
		(stageData_.hasPlayerStartPosition &&
			IsPositionInCell(stageData_.playerStartPosition, column, row)) ||
		(stageData_.hasGoalPosition &&
			IsPositionInCell(stageData_.goalPosition, column, row)))
	{
		return false;
	}
	stageData_.sawSpawns.push_back({});
	stageData_.sawSpawnCount =
		static_cast<int>(stageData_.sawSpawns.size());
	SawSpawnData& saw = stageData_.sawSpawns.back();
	saw.position = {
		(column + 0.5f) * StageData::TileSize,
		(row + 0.5f) * StageData::TileSize
	};
	saw.flipY = IsCeilingPlacement(column, row);
	return true;
}

bool EditorStageData::MoveSaw(int fromColumn, int fromRow,
	int toColumn, int toRow)
{
	const int index = FindSawAt(fromColumn, fromRow);
	if (index < 0 || (fromColumn == toColumn && fromRow == toRow) ||
		!IsInsideStage(toColumn, toRow) || HasSawAt(toColumn, toRow) ||
		StageData::IsSolidTile(stageData_.tiles[toRow][toColumn]) ||
		HasEnemyAt(toColumn, toRow) || HasGravityBlockAt(toColumn, toRow) ||
		HasCheckpointAt(toColumn, toRow) ||
		(stageData_.hasPlayerStartPosition &&
			IsPositionInCell(stageData_.playerStartPosition, toColumn, toRow)) ||
		(stageData_.hasGoalPosition &&
			IsPositionInCell(stageData_.goalPosition, toColumn, toRow)))
	{
		return false;
	}
	SawSpawnData& saw = stageData_.sawSpawns[index];
	const bool changedSurface =
		IsCeilingPlacement(fromColumn, fromRow) !=
		IsCeilingPlacement(toColumn, toRow);
	saw.position = {
		(toColumn + 0.5f) * StageData::TileSize,
		(toRow + 0.5f) * StageData::TileSize
	};
	if (changedSurface)
	{
		saw.flipY = IsCeilingPlacement(toColumn, toRow);
	}
	return true;
}

bool EditorStageData::CanMoveObjectTo(int column, int row,
	float width, float height, int ignoredEnemyIndex, int ignoredBlockIndex,
	bool ignoreMidBoss, bool ignoreBoss, bool forceFloor) const
{
	if (!IsInsideStage(column, row) || HasEnemyAt(column, row) ||
		HasGravityBlockAt(column, row) || HasSawAt(column, row) ||
		HasCheckpointAt(column, row) ||
		(stageData_.hasPlayerStartPosition &&
			IsPositionInCell(stageData_.playerStartPosition, column, row)) ||
		(stageData_.hasGoalPosition &&
			IsPositionInCell(stageData_.goalPosition, column, row)))
	{
		return false;
	}

	const float left = column * StageData::TileSize +
		(StageData::TileSize - width) * 0.5f;
	const float top = !forceFloor && IsCeilingPlacement(column, row)
		? static_cast<float>(row * StageData::TileSize)
		: static_cast<float>((row + 1) * StageData::TileSize) - height;
	const float right = left + width;
	const float bottom = top + height;
	if (left < 0.0f || top < 0.0f ||
		right > StageData::StagePixelWidth ||
		bottom > stageData_.GetPixelHeight())
	{
		return false;
	}

	// 配置マスだけでなく、本体がまたがる地形も確認する。
	for (int tileRow = static_cast<int>(top / StageData::TileSize);
		tileRow < static_cast<int>(std::ceil(bottom / StageData::TileSize));
		++tileRow)
	{
		for (int tileColumn = static_cast<int>(left / StageData::TileSize);
			tileColumn < static_cast<int>(std::ceil(right / StageData::TileSize));
			++tileColumn)
		{
			if (StageData::IsSolidTile(stageData_.tiles[tileRow][tileColumn]))
			{
				return false;
			}
		}
	}
	for (int i = 0; i < stageData_.enemySpawnCount; ++i)
	{
		if (i == ignoredEnemyIndex)
		{
			continue;
		}
		const EnemySpawnData& enemy = stageData_.enemySpawns[i];
		const float enemySize = GetEditorEnemySize(enemy.type);
		if (left < enemy.position.x + enemySize && right > enemy.position.x &&
			top < enemy.position.y + enemySize && bottom > enemy.position.y)
		{
			return false;
		}
	}
	for (int i = 0; i < stageData_.gravityBlockSpawnCount; ++i)
	{
		if (i == ignoredBlockIndex)
		{
			continue;
		}
		const Vector2& blockPosition = stageData_.gravityBlockSpawns[i].position;
		if (left < blockPosition.x + 64.0f && right > blockPosition.x &&
			top < blockPosition.y + 64.0f && bottom > blockPosition.y)
		{
			return false;
		}
	}
	if (stageData_.hasMidBossSpawn && !ignoreMidBoss)
	{
		const Vector2& bossPosition = stageData_.midBossSpawn.position;
		if (left < bossPosition.x + MidBossSpawnData::Size &&
			right > bossPosition.x &&
			top < bossPosition.y + MidBossSpawnData::Size &&
			bottom > bossPosition.y)
		{
			return false;
		}
	}
	if (stageData_.hasBossSpawn && !ignoreBoss)
	{
		const Vector2& bossPosition = stageData_.bossSpawn.position;
		if (left < bossPosition.x + BossSpawnData::Width &&
			right > bossPosition.x &&
			top < bossPosition.y + BossSpawnData::Height &&
			bottom > bossPosition.y)
		{
			return false;
		}
	}
	return true;
}

bool EditorStageData::MoveEnemy(int fromColumn, int fromRow,
	int toColumn, int toRow)
{
	const int index = FindEnemyAt(fromColumn, fromRow);
	const float size = index >= 0
		? GetEditorEnemySize(stageData_.enemySpawns[index].type) : 48.0f;
	if (index < 0 || (fromColumn == toColumn && fromRow == toRow) ||
		!CanMoveObjectTo(toColumn, toRow, size, size, index, -1))
	{
		return false;
	}

	EnemySpawnData& spawnData = stageData_.enemySpawns[index];
	const bool isCeiling = IsCeilingPlacement(toColumn, toRow);
	spawnData.position.x = toColumn * StageData::TileSize +
		(StageData::TileSize - size) * 0.5f;
	spawnData.position.y = isCeiling
		? static_cast<float>(toRow * StageData::TileSize)
		: static_cast<float>((toRow + 1) * StageData::TileSize) - size;
	spawnData.surface = isCeiling
		? EnemySpawnSurface::Ceiling : EnemySpawnSurface::Floor;
	return true;
}

bool EditorStageData::MoveGravityBlock(int fromColumn, int fromRow,
	int toColumn, int toRow)
{
	const int index = FindGravityBlockAt(fromColumn, fromRow);
	if (index < 0 || (fromColumn == toColumn && fromRow == toRow) ||
		!CanMoveObjectTo(toColumn, toRow, 64.0f, 64.0f, -1, index))
	{
		return false;
	}

	GravityBlockSpawnData& spawnData = stageData_.gravityBlockSpawns[index];
	constexpr float blockSize = 64.0f;
	const bool isCeiling = IsCeilingPlacement(toColumn, toRow);
	spawnData.position.x = toColumn * StageData::TileSize +
		(StageData::TileSize - blockSize) * 0.5f;
	spawnData.position.y = isCeiling
		? static_cast<float>(toRow * StageData::TileSize)
		: static_cast<float>((toRow + 1) * StageData::TileSize) - blockSize;
	spawnData.surface = isCeiling
		? GravityBlockSpawnSurface::Ceiling : GravityBlockSpawnSurface::Floor;
	return true;
}

int EditorStageData::GetEnemyHitPointAt(int column, int row) const
{
	const int enemyIndex = FindEnemyAt(column, row);
	return enemyIndex >= 0
		? stageData_.enemySpawns[enemyIndex].hitPoint
		: 0;
}

bool EditorStageData::ChangeEnemyHitPointAt(
	int column,
	int row,
	int amount)
{
	const int enemyIndex = FindEnemyAt(column, row);

	if (enemyIndex < 0 || amount == 0)
	{
		return false;
	}

	EnemySpawnData& spawnData = stageData_.enemySpawns[enemyIndex];
	int newHitPoint = spawnData.hitPoint + amount;

	if (newHitPoint < 1)
	{
		newHitPoint = 1;
	}
	else if (newHitPoint > 99)
	{
		newHitPoint = 99;
	}

	if (newHitPoint == spawnData.hitPoint)
	{
		return false;
	}

	spawnData.hitPoint = newHitPoint;
	return true;
}

int EditorStageData::GetBossHitPointAt(int column, int row) const
{
	if (HasMidBossAt(column, row))
	{
		return stageData_.midBossSpawn.hitPoint;
	}
	if (HasBossAt(column, row))
	{
		return stageData_.bossSpawn.hitPoint;
	}
	return 0;
}

bool EditorStageData::ChangeBossHitPointAt(
	int column,
	int row,
	int amount)
{
	int* hitPoint = nullptr;
	if (HasMidBossAt(column, row))
	{
		hitPoint = &stageData_.midBossSpawn.hitPoint;
	}
	else if (HasBossAt(column, row))
	{
		hitPoint = &stageData_.bossSpawn.hitPoint;
	}
	if (hitPoint == nullptr || amount == 0)
	{
		return false;
	}

	const int newHitPoint = (std::max)(1, (std::min)(99,
		*hitPoint + amount));
	if (newHitPoint == *hitPoint)
	{
		return false;
	}
	*hitPoint = newHitPoint;
	return true;
}

bool EditorStageData::IsEnemyAffectedByGravityReversalAt(
	int column,
	int row) const
{
	const int enemyIndex = FindEnemyAt(column, row);
	return enemyIndex >= 0 &&
		stageData_.enemySpawns[enemyIndex].isAffectedByGravityReversal;
}

bool EditorStageData::ToggleEnemyGravityResponseAt(int column, int row)
{
	const int enemyIndex = FindEnemyAt(column, row);
	if (enemyIndex < 0 ||
		stageData_.enemySpawns[enemyIndex].type == EnemyType::Charger ||
		stageData_.enemySpawns[enemyIndex].type == EnemyType::Mortar)
	{
		return false;
	}

	EnemySpawnData& spawnData = stageData_.enemySpawns[enemyIndex];
	spawnData.isAffectedByGravityReversal =
		!spawnData.isAffectedByGravityReversal;
	return true;
}

bool EditorStageData::IsEnemyFacingRightAt(int column, int row) const
{
	const int enemyIndex = FindEnemyAt(column, row);
	return enemyIndex >= 0 &&
		stageData_.enemySpawns[enemyIndex].facing == EnemySpawnFacing::Right;
}

bool EditorStageData::ToggleEnemyFacingAt(int column, int row)
{
	const int enemyIndex = FindEnemyAt(column, row);
	if (enemyIndex < 0)
	{
		return false;
	}

	EnemySpawnData& spawnData = stageData_.enemySpawns[enemyIndex];
	spawnData.facing = spawnData.facing == EnemySpawnFacing::Left
		? EnemySpawnFacing::Right : EnemySpawnFacing::Left;
	return true;
}

bool EditorStageData::AddFallHole(
	int startColumn, int endColumn, int surfaceRow)
{
	if (stageData_.tileRowCount < 1)
	{
		return false;
	}

	// 左右端の壁を残すため、穴は内側の列だけに制限する。
	if (startColumn < 1)
	{
		startColumn = 1;
	}
	if (endColumn < 1)
	{
		endColumn = 1;
	}
	if (startColumn > StageData::TileColumns - 2)
	{
		startColumn = StageData::TileColumns - 2;
	}
	if (endColumn > StageData::TileColumns - 2)
	{
		endColumn = StageData::TileColumns - 2;
	}

	if (startColumn > endColumn)
	{
		const int temporary = startColumn;
		startColumn = endColumn;
		endColumn = temporary;
	}

	// 2引数で呼ぶ既存コードは、従来どおり床側へ穴を作る。
	const bool usesDefaultSurface = surfaceRow < 0;
	if (usesDefaultSurface)
	{
		surfaceRow = stageData_.tileRowCount - 2;
	}
	if (surfaceRow >= stageData_.tileRowCount ||
		(!usesDefaultSurface &&
			!StageData::IsSolidTile(stageData_.tiles[surfaceRow][startColumn])))
	{
		return false;
	}

	const bool isOpenAbove = usesDefaultSurface || surfaceRow == 0 ||
		!StageData::IsSolidTile(stageData_.tiles[surfaceRow - 1][startColumn]);
	const bool isOpenBelow = surfaceRow == stageData_.tileRowCount - 1 ||
		!StageData::IsSolidTile(stageData_.tiles[surfaceRow + 1][startColumn]);
	if (!isOpenAbove && !isOpenBelow)
	{
		return false;
	}

	int firstRow = surfaceRow;
	int lastRow = usesDefaultSurface
		? stageData_.tileRowCount - 1 : surfaceRow;
	if (!usesDefaultSurface)
	{
		if (isOpenAbove)
		{
			while (lastRow + 1 < stageData_.tileRowCount &&
				StageData::IsSolidTile(stageData_.tiles[lastRow + 1][startColumn]))
			{
				++lastRow;
			}
		}
		else
		{
			while (firstRow > 0 &&
				StageData::IsSolidTile(stageData_.tiles[firstRow - 1][startColumn]))
			{
				--firstRow;
			}
		}
	}

	int mergedLeft = startColumn * StageData::TileSize;
	int mergedRight = (endColumn + 1) * StageData::TileSize;
	const int holeTop = firstRow * StageData::TileSize;
	const int holeBottom = (lastRow + 1) * StageData::TileSize;
	bool overlapsExistingHole = false;

	for (int i = 0; i < stageData_.fallHoleCount; ++i)
	{
		const FallHole& hole = stageData_.fallHoles[i];
		if (hole.top == holeTop && hole.bottom == holeBottom &&
			mergedRight >= hole.left && mergedLeft <= hole.right)
		{
			overlapsExistingHole = true;
		}
	}

	if (!overlapsExistingHole &&
		stageData_.fallHoleCount >= StageData::MaxFallHoles)
	{
		return false;
	}

	// 重なる穴を後ろから取り除き、新しい範囲へまとめる。
	for (int i = stageData_.fallHoleCount - 1; i >= 0; --i)
	{
		const FallHole hole = stageData_.fallHoles[i];
		if (hole.top != holeTop || hole.bottom != holeBottom ||
			mergedRight < hole.left || mergedLeft > hole.right)
		{
			continue;
		}

		if (hole.left < mergedLeft)
		{
			mergedLeft = hole.left;
		}
		if (hole.right > mergedRight)
		{
			mergedRight = hole.right;
		}

		for (int moveIndex = i;
			moveIndex + 1 < stageData_.fallHoleCount;
			++moveIndex)
		{
			stageData_.fallHoles[moveIndex] =
				stageData_.fallHoles[moveIndex + 1];
		}
		--stageData_.fallHoleCount;
	}

	int insertIndex = stageData_.fallHoleCount;
	while (insertIndex > 0 &&
		(stageData_.fallHoles[insertIndex - 1].top > holeTop ||
			(stageData_.fallHoles[insertIndex - 1].top == holeTop &&
				stageData_.fallHoles[insertIndex - 1].left > mergedLeft)))
	{
		stageData_.fallHoles[insertIndex] =
			stageData_.fallHoles[insertIndex - 1];
		--insertIndex;
	}

	stageData_.fallHoles[insertIndex] = {
		mergedLeft, mergedRight, holeTop, holeBottom
	};
	++stageData_.fallHoleCount;

	const int firstColumn = mergedLeft / StageData::TileSize;
	const int lastColumn = mergedRight / StageData::TileSize;
	for (int row = firstRow; row <= lastRow; ++row)
	{
		for (int column = firstColumn; column < lastColumn; ++column)
		{
			stageData_.tiles[row][column] = StageData::EmptyTile;
			stageData_.solidVisuals[row][column] = {};
		}
	}

	return true;
}

bool EditorStageData::RemoveFallHoleAt(int column, int row)
{
	if (stageData_.tileRowCount < 4)
	{
		return false;
	}

	const int worldX = column * StageData::TileSize + StageData::TileSize / 2;
	const int worldY = row * StageData::TileSize + StageData::TileSize / 2;
	int removeIndex = -1;

	for (int i = 0; i < stageData_.fallHoleCount; ++i)
	{
		const FallHole& hole = stageData_.fallHoles[i];
		const bool matchesRow = row < 0 || hole.IsLegacyBoundaryHole() ||
			(worldY >= hole.top && worldY < hole.bottom);
		if (worldX >= hole.left && worldX < hole.right && matchesRow)
		{
			removeIndex = i;
			break;
		}
	}

	if (removeIndex < 0)
	{
		return false;
	}

	const FallHole removedHole = stageData_.fallHoles[removeIndex];
	for (int i = removeIndex; i + 1 < stageData_.fallHoleCount; ++i)
	{
		stageData_.fallHoles[i] = stageData_.fallHoles[i + 1];
	}
	--stageData_.fallHoleCount;
	stageData_.fallHoles[stageData_.fallHoleCount] = {};

	const int firstColumn = removedHole.left / StageData::TileSize;
	const int lastColumn = removedHole.right / StageData::TileSize;
	int firstRow = removedHole.top / StageData::TileSize;
	int lastRow = removedHole.bottom / StageData::TileSize;
	if (removedHole.IsLegacyBoundaryHole())
	{
		firstRow = row >= stageData_.tileRowCount / 2
			? stageData_.tileRowCount - 2 : 0;
		lastRow = firstRow + 2;
	}
	for (int fillRow = firstRow; fillRow < lastRow; ++fillRow)
	{
		for (int fillColumn = firstColumn; fillColumn < lastColumn; ++fillColumn)
		{
			stageData_.tiles[fillRow][fillColumn] = StageData::FloorTile;
			stageData_.solidVisuals[fillRow][fillColumn] = {};
		}
	}

	return true;
}

bool EditorStageData::IsInsideStage(int column, int row) const
{
	return column >= 0 && column < StageData::TileColumns &&
		row >= 0 && row < stageData_.tileRowCount;
}

bool EditorStageData::IsPositionInCell(
	const Vector2& position,
	int column,
	int row) const
{
	return static_cast<int>(position.x) / StageData::TileSize == column &&
		static_cast<int>(position.y) / StageData::TileSize == row;
}

// すぐ上だけが地形なら天井側、それ以外は床側の配置として扱う。
bool EditorStageData::IsCeilingPlacement(int column, int row) const
{
	const bool hasSolidAbove =
		row > 0 &&
		StageData::IsSolidTile(stageData_.tiles[row - 1][column]);
	const bool hasSolidBelow =
		row + 1 < stageData_.tileRowCount &&
		StageData::IsSolidTile(stageData_.tiles[row + 1][column]);

	return hasSolidAbove && !hasSolidBelow;
}

// ブロックの中心と接地辺から、エディター上の配置マスを逆算する。
int EditorStageData::FindGravityBlockAt(int column, int row) const
{
	constexpr int blockSize = 64;

	for (int i = 0; i < stageData_.gravityBlockSpawnCount; ++i)
	{
		const GravityBlockSpawnData& spawnData =
			stageData_.gravityBlockSpawns[i];
		const int blockColumn =
			(static_cast<int>(spawnData.position.x) + blockSize / 2) /
			StageData::TileSize;
		const int blockRow =
			spawnData.surface == GravityBlockSpawnSurface::Ceiling
				? static_cast<int>(spawnData.position.y) / StageData::TileSize
				: (static_cast<int>(spawnData.position.y) + blockSize - 1) /
					StageData::TileSize;

		if (blockColumn == column && blockRow == row)
		{
			return i;
		}
	}

	return -1;
}

bool EditorStageData::RemoveGravityBlockAt(int column, int row)
{
	const int removeIndex = FindGravityBlockAt(column, row);

	if (removeIndex < 0)
	{
		return false;
	}

	for (int i = removeIndex;
		i + 1 < stageData_.gravityBlockSpawnCount;
		++i)
	{
		stageData_.gravityBlockSpawns[i] =
			stageData_.gravityBlockSpawns[i + 1];
	}

	--stageData_.gravityBlockSpawnCount;
	stageData_.gravityBlockSpawns[stageData_.gravityBlockSpawnCount] = {};
	return true;
}

bool EditorStageData::PlaceMovingGravityBlock(int column, int row)
{
	return PlaceGravityBlock(column, row, GravityBlockMotion::Vertical);
}

bool EditorStageData::PlaceGravityBlock(
	int column,
	int row,
	GravityBlockMotion motion)
{
	const int existingIndex = FindGravityBlockAt(column, row);

	// 配置上限に達している場合でも、同じマスの置き直しは許可する。
	if (existingIndex < 0 &&
		stageData_.gravityBlockSpawnCount >= StageData::MaxGravityBlockSpawns)
	{
		return false;
	}

	bool hasChanged = RemoveGravityBlockAt(column, row);

	if (stageData_.hasPlayerStartPosition &&
		IsPositionInCell(stageData_.playerStartPosition, column, row))
	{
		stageData_.hasPlayerStartPosition = false;
		hasChanged = true;
	}

	if (stageData_.hasGoalPosition &&
		IsPositionInCell(stageData_.goalPosition, column, row))
	{
		stageData_.hasGoalPosition = false;
		hasChanged = true;
	}

	hasChanged = SetTile(column, row, StageData::EmptyTile) || hasChanged;
	hasChanged = RemoveEnemyAt(column, row) || hasChanged;
	hasChanged = RemoveSawAt(column, row) || hasChanged;
	hasChanged = RemoveCheckpointAt(column, row) || hasChanged;

	constexpr float blockSize = 64.0f;
	const bool isCeiling = IsCeilingPlacement(column, row);
	GravityBlockSpawnData& spawnData =
		stageData_.gravityBlockSpawns[stageData_.gravityBlockSpawnCount];
	spawnData.position.x =
		column * StageData::TileSize +
		(StageData::TileSize - blockSize) * 0.5f;
	spawnData.position.y = isCeiling
		? static_cast<float>(row * StageData::TileSize)
		: static_cast<float>(row * StageData::TileSize) +
			StageData::TileSize - blockSize;
	spawnData.surface = isCeiling
		? GravityBlockSpawnSurface::Ceiling
		: GravityBlockSpawnSurface::Floor;
	spawnData.motion = motion;
	++stageData_.gravityBlockSpawnCount;

	return true;
}

GravityBlockMotion EditorStageData::GetGravityBlockMotionAt(
	int column,
	int row) const
{
	const int index = FindGravityBlockAt(column, row);
	return index >= 0
		? stageData_.gravityBlockSpawns[index].motion
		: GravityBlockMotion::Gravity;
}

bool EditorStageData::ToggleGravityBlockMotionAt(int column, int row)
{
	const int index = FindGravityBlockAt(column, row);
	if (index < 0)
	{
		return false;
	}

	GravityBlockMotion& motion = stageData_.gravityBlockSpawns[index].motion;
	if (motion == GravityBlockMotion::Gravity)
	{
		return false;
	}

	motion = motion == GravityBlockMotion::Vertical
		? GravityBlockMotion::Horizontal
		: GravityBlockMotion::Vertical;
	return true;
}

int EditorStageData::FindEnemyAt(int column, int row) const
{
	for (int i = 0; i < stageData_.enemySpawnCount; ++i)
	{
		const EnemySpawnData& spawnData = stageData_.enemySpawns[i];
		const int enemySize = static_cast<int>(GetEditorEnemySize(spawnData.type));
		const int enemyColumn =
			(static_cast<int>(spawnData.position.x) + enemySize / 2) /
			StageData::TileSize;
		const int enemyRow =
			spawnData.surface == EnemySpawnSurface::Ceiling
				? static_cast<int>(spawnData.position.y) / StageData::TileSize
				: (static_cast<int>(spawnData.position.y) + enemySize - 1) /
					StageData::TileSize;

		if (enemyColumn == column && enemyRow == row)
		{
			return i;
		}
	}

	return -1;
}

bool EditorStageData::RemoveEnemyAt(int column, int row)
{
	const int removeIndex = FindEnemyAt(column, row);

	if (removeIndex < 0)
	{
		return false;
	}

	for (int i = removeIndex; i + 1 < stageData_.enemySpawnCount; ++i)
	{
		stageData_.enemySpawns[i] = stageData_.enemySpawns[i + 1];
	}

	--stageData_.enemySpawnCount;
	stageData_.enemySpawns[stageData_.enemySpawnCount] = {};
	return true;
}

bool EditorStageData::PlaceEnemy(int column, int row, char item)
{
	const int existingIndex = FindEnemyAt(column, row);

	if (existingIndex < 0 &&
		stageData_.enemySpawnCount >= StageData::MaxEnemySpawns)
	{
		return false;
	}

	bool hasChanged = RemoveEnemyAt(column, row);

	if (stageData_.hasPlayerStartPosition &&
		IsPositionInCell(stageData_.playerStartPosition, column, row))
	{
		stageData_.hasPlayerStartPosition = false;
		hasChanged = true;
	}

	if (stageData_.hasGoalPosition &&
		IsPositionInCell(stageData_.goalPosition, column, row))
	{
		stageData_.hasGoalPosition = false;
		hasChanged = true;
	}

	hasChanged = RemoveGravityBlockAt(column, row) || hasChanged;
	hasChanged = RemoveSawAt(column, row) || hasChanged;
	hasChanged = RemoveCheckpointAt(column, row) || hasChanged;
	hasChanged = SetTile(column, row, StageData::EmptyTile) || hasChanged;

	EnemyType enemyType = EnemyType::Normal;
	int hitPoint = 1;
	float enemySize = 48.0f;

	if (item == StageData::HeavyEnemyTile)
	{
		enemyType = EnemyType::Heavy;
		hitPoint = 2;
		enemySize = 56.0f;
	}
	else if (item == StageData::ShooterEnemyTile)
	{
		enemyType = EnemyType::Shooter;
	}
	else if (item == StageData::UpShooterEnemyTile)
	{
		enemyType = EnemyType::UpShooter;
	}
	else if (item == StageData::ChargerEnemyTile)
	{
		enemyType = EnemyType::Charger;
		enemySize = 80.0f;
	}
	else if (item == StageData::MortarEnemyTile)
	{
		enemyType = EnemyType::Mortar;
		enemySize = 60.0f;
	}

	const bool isCeiling = IsCeilingPlacement(column, row);
	EnemySpawnData& spawnData =
		stageData_.enemySpawns[stageData_.enemySpawnCount];
	spawnData.position.x =
		column * StageData::TileSize +
		(StageData::TileSize - enemySize) * 0.5f;
	spawnData.position.y = isCeiling
		? static_cast<float>(row * StageData::TileSize)
		: static_cast<float>(row * StageData::TileSize) +
			StageData::TileSize - enemySize;
	spawnData.hitPoint = hitPoint;
	spawnData.surface = isCeiling
		? EnemySpawnSurface::Ceiling
		: EnemySpawnSurface::Floor;
	spawnData.type = enemyType;
	if (enemyType == EnemyType::Charger || enemyType == EnemyType::Mortar)
	{
		spawnData.isAffectedByGravityReversal = false;
	}
	++stageData_.enemySpawnCount;

	return true;
}

// 範囲内のマスだけを書き換え、同じタイルの場合は変更なしとして扱う。
bool EditorStageData::SetTile(int column, int row, char tile)
{
	if (!IsInsideStage(column, row))
	{
		return false;
	}

	const bool changed = stageData_.tiles[row][column] != tile ||
		stageData_.solidVisuals[row][column].sheetIndex >= 0;
	stageData_.tiles[row][column] = tile;
	stageData_.solidVisuals[row][column] = {};
	return changed;
}
