#include "StageFileWriter.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace
{
	std::string BuildStageFilePath(int stageNumber)
	{
		return "Assets/Stages/stage" +
			std::to_string(stageNumber) + ".txt";
	}

	// 元ファイルを直前の保存内容として残す。
	bool CopyFile(const std::string& sourcePath, const std::string& destinationPath)
	{
		std::ifstream sourceFile(sourcePath, std::ios::binary);
		std::ofstream destinationFile(
			destinationPath,
			std::ios::binary | std::ios::trunc);

		if (!sourceFile.is_open() || !destinationFile.is_open())
		{
			return false;
		}

		destinationFile << sourceFile.rdbuf();
		return sourceFile.good() || sourceFile.eof()
			? destinationFile.good()
			: false;
	}

	const char* GetGravityBlockSurfaceText(GravityBlockSpawnSurface surface)
	{
		return surface == GravityBlockSpawnSurface::Ceiling
			? "Ceiling"
			: "Floor";
	}

	const char* GetGravityBlockMotionText(GravityBlockMotion motion)
	{
		switch (motion)
		{
		case GravityBlockMotion::Vertical:
			return "Vertical";

		case GravityBlockMotion::Horizontal:
			return "Horizontal";

		default:
			return "Gravity";
		}
	}

	const char* GetEnemyTypeText(EnemyType type)
	{
		switch (type)
		{
		case EnemyType::Heavy:
			return "Heavy";

		case EnemyType::Shooter:
			return "Shooter";

		case EnemyType::UpShooter:
			return "UpShooter";

		case EnemyType::Charger:
			return "Charger";

		case EnemyType::Mortar:
			return "Mortar";

		default:
			return "Normal";
		}
	}

	const char* GetEnemySurfaceText(EnemySpawnSurface surface)
	{
		return surface == EnemySpawnSurface::Ceiling
			? "Ceiling"
			: "Floor";
	}

	const char* GetEnemyGravityResponseText(
		bool isAffectedByGravityReversal)
	{
		return isAffectedByGravityReversal ? "Affected" : "Fixed";
	}

	const char* GetEnemyFacingText(EnemySpawnFacing facing)
	{
		return facing == EnemySpawnFacing::Right ? "Right" : "Left";
	}

	const char* GetCameraAreaModeText(CameraAreaMode mode)
	{
		switch (mode)
		{
		case CameraAreaMode::Horizontal:
			return "Horizontal";

		case CameraAreaMode::Vertical:
			return "Vertical";

		case CameraAreaMode::Locked:
			return "Locked";

		default:
			return "Free";
		}
	}

	// 一時ファイルへ、ローダーが読めるステージ形式をすべて書き出す。
	bool WriteStageFile(
		const std::string& filePath,
		const StageData& stageData)
	{
		std::ofstream file(filePath, std::ios::trunc);

		if (!file.is_open())
		{
			return false;
		}

		for (int row = 0; row < stageData.tileRowCount; ++row)
		{
			for (int column = 0; column < StageData::TileColumns; ++column)
			{
				file << stageData.tiles[row][column];
			}

			file << '\n';
		}

		file << "\n[FallHoles]\n";
		for (int i = 0; i < stageData.fallHoleCount; ++i)
		{
			const FallHole& hole = stageData.fallHoles[i];
			file << hole.left << ' ' << hole.right;
			if (!hole.IsLegacyBoundaryHole())
			{
				file << ' ' << hole.top << ' ' << hole.bottom;
			}
			file << '\n';
		}

		file << "\n[GravityBlocks]\n";
		for (int i = 0; i < stageData.gravityBlockSpawnCount; ++i)
		{
			const GravityBlockSpawnData& spawnData =
				stageData.gravityBlockSpawns[i];
			file << spawnData.position.x << ' '
				<< spawnData.position.y << ' '
				<< GetGravityBlockSurfaceText(spawnData.surface) << ' '
				<< GetGravityBlockMotionText(spawnData.motion) << '\n';
		}

		if (stageData.hasGoalPosition)
		{
			file << "\n[Goal]\n"
				<< stageData.goalPosition.x << ' '
				<< stageData.goalPosition.y << '\n';
		}

		if (stageData.hasPlayerStartPosition)
		{
			file << "\n[Player]\n"
				<< stageData.playerStartPosition.x << ' '
				<< stageData.playerStartPosition.y << '\n';
		}

		file << "\n[Checkpoints]\n";
		for (int i = 0; i < stageData.checkpointCount; ++i)
		{
			file << stageData.checkpoints[i].x << ' '
				<< stageData.checkpoints[i].y << '\n';
		}

		if (stageData.hasMidBossSpawn)
		{
			const MidBossSpawnData& spawn = stageData.midBossSpawn;
			file << "\n[MidBoss]\n"
				<< "TurtleGrenadier "
				<< spawn.position.x << ' '
				<< spawn.position.y << ' '
				<< GetEnemySurfaceText(spawn.surface) << ' '
				<< GetEnemyFacingText(spawn.facing) << ' '
				<< spawn.cameraGroupId << ' '
				<< spawn.hitPoint << '\n';
		}

		if (stageData.hasBossSpawn)
		{
			const BossSpawnData& spawn = stageData.bossSpawn;
			file << "\n[Boss]\n"
				<< "Tank "
				<< spawn.position.x << ' '
				<< spawn.position.y << ' '
				<< GetEnemyFacingText(spawn.facing) << ' '
				<< spawn.cameraGroupId << ' '
				<< spawn.hitPoint << '\n';
		}

		// 0体でも空のセクションを残し、コード側の仮配置へ戻らないようにする。
		file << "\n[Enemies]\n";
		for (int i = 0; i < stageData.enemySpawnCount; ++i)
		{
			const EnemySpawnData& spawnData = stageData.enemySpawns[i];
			file << GetEnemyTypeText(spawnData.type) << ' '
				<< spawnData.position.x << ' '
				<< spawnData.position.y << ' '
				<< spawnData.hitPoint << ' '
				<< GetEnemySurfaceText(spawnData.surface) << ' '
				<< GetEnemyGravityResponseText(
					spawnData.isAffectedByGravityReversal) << ' '
				<< GetEnemyFacingText(spawnData.facing) << '\n';
		}

		file << "\n[CameraAreas]\n";
		for (int i = 0; i < stageData.cameraAreaCount; ++i)
		{
			const CameraArea& area = stageData.cameraAreas[i];
			file << GetCameraAreaModeText(area.mode) << ' '
				<< area.left << ' '
				<< area.top << ' '
				<< area.right << ' '
				<< area.bottom << ' '
				<< (area.groupId >= 0 ? area.groupId : i) << '\n';
		}

		file << "\n[Saws]\n";
		for (int i = 0; i < stageData.sawSpawnCount; ++i)
		{
			const SawSpawnData& saw = stageData.sawSpawns[i];
			file << saw.position.x << ' ' << saw.position.y << ' '
				<< static_cast<int>(saw.flipX) << ' '
				<< static_cast<int>(saw.flipY) << '\n';
		}

		// 画像名を先に列挙し、各マスは番号と切り出し座標だけを保存する。
		if (stageData.tileSheetCount > 0)
		{
			file << "\n[TileSheets]\n";
			for (int i = 0; i < stageData.tileSheetCount; ++i)
			{
				file << stageData.tileSheetNames[i] << '\n';
			}
			file << "\n[VisualTiles]\n";
			for (int row = 0; row < stageData.tileRowCount; ++row)
			{
				for (int column = 0; column < StageData::TileColumns; ++column)
				{
					const VisualTile& solid = stageData.solidVisuals[row][column];
					const VisualTile& background = stageData.backgroundVisuals[row][column];
					if (StageData::IsSolidTile(stageData.tiles[row][column]) &&
						solid.sheetIndex >= 0 && solid.sheetIndex < stageData.tileSheetCount)
					{
						file << 'S' << ' ' << column << ' ' << row << ' '
							<< solid.sheetIndex << ' '
							<< static_cast<int>(solid.column) << ' '
							<< static_cast<int>(solid.row) << ' '
							<< static_cast<int>(solid.frameCount) << ' '
							<< static_cast<int>(solid.flipX) << ' '
							<< static_cast<int>(solid.flipY) << '\n';
					}
					if (background.sheetIndex >= 0 &&
						background.sheetIndex < stageData.tileSheetCount)
					{
						file << 'B' << ' ' << column << ' ' << row << ' '
							<< background.sheetIndex << ' '
							<< static_cast<int>(background.column) << ' '
							<< static_cast<int>(background.row) << ' '
							<< static_cast<int>(background.frameCount) << ' '
							<< static_cast<int>(background.flipX) << ' '
							<< static_cast<int>(background.flipY) << '\n';
					}
				}
			}
		}

		file.flush();
		return file.good();
	}
}

bool StageFileWriter::Save(const StageData& stageData)
{
	const std::string filePath =
		BuildStageFilePath(stageData.stageNumber);
	const std::string backupPath = filePath + ".bak";
	const std::string temporaryPath = filePath + ".tmp";

	if (!CopyFile(filePath, backupPath))
	{
		return false;
	}

	if (!WriteStageFile(temporaryPath, stageData))
	{
		std::remove(temporaryPath.c_str());
		return false;
	}

	// Windowsでは既存ファイルへrenameできないため、バックアップ後に置き換える。
	if (std::remove(filePath.c_str()) != 0)
	{
		std::remove(temporaryPath.c_str());
		return false;
	}

	if (std::rename(temporaryPath.c_str(), filePath.c_str()) != 0)
	{
		CopyFile(backupPath, filePath);
		std::remove(temporaryPath.c_str());
		return false;
	}

	return true;
}
