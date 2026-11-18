#include "StageFileLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string BuildStageFilePath(int stageNumber)
	{
		return "Assets/Stages/stage" +
			std::to_string(stageNumber) + ".txt";
	}

	// ファイル先頭から、空行またはセクション行までをタイルマップとして読む。
	// 行数を固定しないことで、横通路と縦通路を組み合わせたステージに対応する。
	bool ReadTileLines(
		int stageNumber,
		std::vector<std::string>& tileLines)
	{
		std::ifstream file(BuildStageFilePath(stageNumber));
		std::string line;

		tileLines.clear();

		if (!file.is_open())
		{
			return false;
		}

		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '[')
			{
				break;
			}

			if (tileLines.size() >= StageData::TileRows)
			{
				break;
			}

			tileLines.push_back(line);
		}

		return !tileLines.empty();
	}

	void WriteStageWarning(
		int stageNumber,
		const std::string& message)
	{
		std::ofstream logFile(
			"Assets/Stages/StageValidationLog.txt",
			std::ios::app);

		if (!logFile.is_open())
		{
			return;
		}

		logFile << "stage" << stageNumber
			<< ": " << message << '\n';
	}

	bool IsKnownTile(char tile)
	{
		return tile == StageData::EmptyTile ||
			tile == StageData::BlockTile ||
			tile == StageData::FloorTile ||
			tile == StageData::WallTile ||
			tile == StageData::InvisibleBlockTile ||
			tile == StageData::PlayerStartTile ||
			tile == StageData::GoalTile ||
			tile == StageData::GravityBlockTile ||
			tile == StageData::NormalEnemyTile ||
			tile == StageData::HeavyEnemyTile ||
			tile == StageData::ShooterEnemyTile ||
			tile == StageData::UpShooterEnemyTile ||
			tile == StageData::ChargerEnemyTile ||
			tile == StageData::MortarEnemyTile ||
			tile == StageData::CheckpointTile;
	}

	// 配置記号の上下に、接地できる地形があるかを調べるために使う。
	bool IsSolidSourceTile(char tile)
	{
		return tile == StageData::BlockTile ||
			tile == StageData::FloorTile ||
			tile == StageData::WallTile ||
			tile == StageData::InvisibleBlockTile;
	}

	// 配置記号のすぐ上だけに地形があれば、天井側の配置と判断する。
	bool IsCeilingObjectMarker(
		const std::vector<std::string>& tileLines,
		int x,
		int y)
	{
		const bool hasSolidAbove =
			y > 0 &&
			x < static_cast<int>(tileLines[y - 1].size()) &&
			IsSolidSourceTile(tileLines[y - 1][x]);
		const bool hasSolidBelow =
			y + 1 < static_cast<int>(tileLines.size()) &&
			x < static_cast<int>(tileLines[y + 1].size()) &&
			IsSolidSourceTile(tileLines[y + 1][x]);

		return hasSolidAbove && !hasSolidBelow;
	}

	bool IsSectionLine(const std::string& line)
	{
		return !line.empty() && line[0] == '[';
	}

	bool TryParseBossHitPoint(const std::string& text, int& hitPoint)
	{
		std::istringstream stream(text);
		std::string extra;
		return static_cast<bool>(stream >> hitPoint) &&
			hitPoint >= 1 && hitPoint <= 99 && !(stream >> extra);
	}

	void ValidateTileLine(
		int stageNumber,
		const std::string& line,
		int rowIndex)
	{
		if (static_cast<int>(line.size()) > StageData::TileColumns)
		{
			WriteStageWarning(
				stageNumber,
				"tile row " + std::to_string(rowIndex + 1) +
				" has " + std::to_string(line.size()) +
				" chars. maximum is " +
				std::to_string(StageData::TileColumns) + ".");
		}

		for (int x = 0; x < static_cast<int>(line.size()); ++x)
		{
			if (!IsKnownTile(line[x]))
			{
				WriteStageWarning(
					stageNumber,
					"unknown tile '" + std::string(1, line[x]) +
					"' at row " + std::to_string(rowIndex + 1) +
					", column " + std::to_string(x + 1) + ".");
			}
		}
	}

	void ValidateFallHoleLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		int left = 0;
		int right = 0;
		int top = 0;
		int bottom = 0;
		std::string extra;

		if (!(lineStream >> left >> right))
		{
			WriteStageWarning(
				stageNumber,
				"[FallHoles] line " + std::to_string(lineNumber) +
				" should be: left right [top bottom].");
			return;
		}

		const bool hasTop = static_cast<bool>(lineStream >> top);
		const bool hasBottom = hasTop &&
			static_cast<bool>(lineStream >> bottom);
		if (hasTop != hasBottom || lineStream >> extra)
		{
			WriteStageWarning(
				stageNumber,
				"[FallHoles] line " + std::to_string(lineNumber) +
				" should be: left right [top bottom].");
			return;
		}

		if (right <= left)
		{
			WriteStageWarning(
				stageNumber,
				"[FallHoles] line " + std::to_string(lineNumber) +
				" has right <= left.");
		}
		if (hasTop && bottom <= top)
		{
			WriteStageWarning(
				stageNumber,
				"[FallHoles] line " + std::to_string(lineNumber) +
				" has bottom <= top.");
		}
	}

	void ValidateGravityBlockLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;
		if (!(lineStream >> x >> y))
		{
			WriteStageWarning(
				stageNumber,
				"[GravityBlocks] line " + std::to_string(lineNumber) +
				" should be: x y [Surface] [Motion].");
			return;
		}

		std::string surfaceText;

		if (lineStream >> surfaceText &&
			surfaceText != "Floor" &&
			surfaceText != "Ceiling")
		{
			WriteStageWarning(
				stageNumber,
				"[GravityBlocks] line " + std::to_string(lineNumber) +
				" has unknown surface: " + surfaceText + ".");
		}

		std::string motionText;
		if (lineStream >> motionText &&
			motionText != "Gravity" &&
			motionText != "Vertical" &&
			motionText != "Horizontal")
		{
			WriteStageWarning(
				stageNumber,
				"[GravityBlocks] line " + std::to_string(lineNumber) +
				" has unknown motion: " + motionText + ".");
		}
	}

	void ValidateGoalLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;
		std::string surfaceText;

		if (!(lineStream >> x >> y))
		{
			WriteStageWarning(
				stageNumber,
				"[Goal] line " + std::to_string(lineNumber) +
				" should be: x y.");
		}
	}

	void ValidatePlayerLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;

		if (!(lineStream >> x >> y))
		{
			WriteStageWarning(
				stageNumber,
				"[Player] line " + std::to_string(lineNumber) +
				" should be: x y.");
		}
	}

	void ValidateCheckpointLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;

		if (!(lineStream >> x >> y))
		{
			WriteStageWarning(
				stageNumber,
				"[Checkpoints] line " + std::to_string(lineNumber) +
				" should be: x y.");
		}
	}

	bool TryParseCameraAreaMode(
		const std::string& text,
		CameraAreaMode& mode)
	{
		if (text == "Horizontal")
		{
			mode = CameraAreaMode::Horizontal;
			return true;
		}

		if (text == "Vertical")
		{
			mode = CameraAreaMode::Vertical;
			return true;
		}

		if (text == "Free")
		{
			mode = CameraAreaMode::Free;
			return true;
		}

		if (text == "Locked")
		{
			mode = CameraAreaMode::Locked;
			return true;
		}

		return false;
	}

	void ValidateCameraAreaLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		std::string modeText;
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		int groupId = -1;

		if (!(lineStream >> modeText >> left >> top >> right >> bottom))
		{
			WriteStageWarning(
				stageNumber,
				"[CameraAreas] line " + std::to_string(lineNumber) +
				" should be: Mode left top right bottom.");
			return;
		}
		lineStream >> groupId;

		CameraAreaMode mode = CameraAreaMode::Free;

		if (!TryParseCameraAreaMode(modeText, mode))
		{
			WriteStageWarning(
				stageNumber,
				"[CameraAreas] line " + std::to_string(lineNumber) +
				" has unknown mode: " + modeText + ".");
		}

		if (right <= left || bottom <= top)
		{
			WriteStageWarning(
				stageNumber,
				"[CameraAreas] line " + std::to_string(lineNumber) +
				" has an invalid rectangle.");
		}
		if (groupId < -1)
		{
			WriteStageWarning(
				stageNumber,
				"[CameraAreas] line " + std::to_string(lineNumber) +
				" has an invalid group id.");
		}
	}

	bool TryParseEnemyType(
		const std::string& text,
		EnemyType& type)
	{
		if (text == "Normal")
		{
			type = EnemyType::Normal;
			return true;
		}

		if (text == "Heavy")
		{
			type = EnemyType::Heavy;
			return true;
		}

		if (text == "Shooter")
		{
			type = EnemyType::Shooter;
			return true;
		}

		if (text == "UpShooter")
		{
			type = EnemyType::UpShooter;
			return true;
		}

		if (text == "Charger")
		{
			type = EnemyType::Charger;
			return true;
		}

		if (text == "Mortar")
		{
			type = EnemyType::Mortar;
			return true;
		}

		return false;
	}

	bool TryParseEnemyFacing(const std::string& text,
		EnemySpawnFacing& facing)
	{
		if (text == "Left")
		{
			facing = EnemySpawnFacing::Left;
			return true;
		}
		if (text == "Right")
		{
			facing = EnemySpawnFacing::Right;
			return true;
		}
		return false;
	}

	bool TryParseEnemySurface(
		const std::string& text,
		EnemySpawnSurface& surface)
	{
		if (text == "Floor")
		{
			surface = EnemySpawnSurface::Floor;
			return true;
		}

		if (text == "Ceiling")
		{
			surface = EnemySpawnSurface::Ceiling;
			return true;
		}

		return false;
	}

	bool TryParseEnemyGravityResponse(
		const std::string& text,
		bool& isAffectedByGravityReversal)
	{
		if (text == "Affected")
		{
			isAffectedByGravityReversal = true;
			return true;
		}

		if (text == "Fixed")
		{
			isAffectedByGravityReversal = false;
			return true;
		}

		return false;
	}

	void ValidateEnemyLine(
		int stageNumber,
		const std::string& line,
		int lineNumber)
	{
		std::istringstream lineStream(line);
		std::string typeText;
		std::string surfaceText;
		std::string gravityResponseText;
		std::string facingText;
		float x = 0.0f;
		float y = 0.0f;
		int hitPoint = 1;

		if (!(lineStream >> typeText >> x >> y >> hitPoint >> surfaceText))
		{
			WriteStageWarning(
				stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" should be: Type x y HP Surface [Affected|Fixed] [Left|Right].");
			return;
		}

		EnemyType type = EnemyType::Normal;
		EnemySpawnSurface surface = EnemySpawnSurface::Floor;
		bool isAffectedByGravityReversal = true;
		EnemySpawnFacing facing = EnemySpawnFacing::Left;

		if (!TryParseEnemyType(typeText, type))
		{
			WriteStageWarning(
				stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" has unknown enemy type: " + typeText + ".");
		}

		if (!TryParseEnemySurface(surfaceText, surface))
		{
			WriteStageWarning(
				stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" has unknown surface: " + surfaceText + ".");
		}

		if (lineStream >> gravityResponseText &&
			!TryParseEnemyGravityResponse(
				gravityResponseText, isAffectedByGravityReversal))
		{
			WriteStageWarning(
				stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" has unknown gravity response: " + gravityResponseText + ".");
		}

		if (lineStream >> facingText && !TryParseEnemyFacing(facingText, facing))
		{
			WriteStageWarning(stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" has unknown facing: " + facingText + ".");
		}

		if (hitPoint <= 0)
		{
			WriteStageWarning(
				stageNumber,
				"[Enemies] line " + std::to_string(lineNumber) +
				" has HP <= 0.");
		}
	}

	void ValidateStageFile(int stageNumber)
	{
		static bool hasClearedValidationLog = false;

		if (!hasClearedValidationLog)
		{
			std::ofstream clearLogFile(
				"Assets/Stages/StageValidationLog.txt",
				std::ios::trunc);
			hasClearedValidationLog = true;
		}

		const std::string filePath =
			BuildStageFilePath(stageNumber);
		std::ifstream file(filePath);

		if (!file.is_open())
		{
			WriteStageWarning(
				stageNumber,
				"stage file could not be opened.");
			return;
		}

		std::string line;
		std::string currentSection;
		int lineNumber = 0;
		int tileRowCount = 0;
		bool isReadingTileMap = true;

		while (std::getline(file, line))
		{
			++lineNumber;

			if (isReadingTileMap && !line.empty() && !IsSectionLine(line))
			{
				if (tileRowCount < StageData::TileRows)
				{
					ValidateTileLine(stageNumber, line, tileRowCount);
				}
				else if (tileRowCount == StageData::TileRows)
				{
					WriteStageWarning(
						stageNumber,
						"tile map exceeds the maximum row count of " +
						std::to_string(StageData::TileRows) + ".");
				}

				++tileRowCount;
				continue;
			}

			if (line.empty())
			{
				isReadingTileMap = false;
				continue;
			}

			if (IsSectionLine(line))
			{
				isReadingTileMap = false;
				currentSection = line;

				if (currentSection != "[FallHoles]" &&
					currentSection != "[GravityBlocks]" &&
					currentSection != "[Saws]" &&
					currentSection != "[Goal]" &&
					currentSection != "[Player]" &&
					currentSection != "[Checkpoints]" &&
					currentSection != "[Enemies]" &&
					currentSection != "[MidBoss]" &&
					currentSection != "[Boss]" &&
					currentSection != "[CameraAreas]" &&
					currentSection != "[TileSheets]" &&
					currentSection != "[VisualTiles]")
				{
					WriteStageWarning(
						stageNumber,
						"unknown section at line " +
						std::to_string(lineNumber) +
						": " + currentSection + ".");
				}

				continue;
			}

			if (currentSection == "[FallHoles]")
			{
				ValidateFallHoleLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[GravityBlocks]")
			{
				ValidateGravityBlockLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[Saws]")
			{
				std::istringstream values(line);
				float x = 0.0f;
				float y = 0.0f;
				int flipX = 0;
				int flipY = 0;
				std::string extra;
				bool valid = static_cast<bool>(values >> x >> y);
				if (valid && values >> flipX)
				{
					valid = static_cast<bool>(values >> flipY) &&
						(flipX == 0 || flipX == 1) &&
						(flipY == 0 || flipY == 1) && !(values >> extra);
				}
				else if (valid && !values.eof())
				{
					valid = false;
				}
				if (!valid)
				{
					WriteStageWarning(stageNumber,
						"[Saws] line " + std::to_string(lineNumber) +
						" should be: centerX centerY [flipX flipY].");
				}
			}
			else if (currentSection == "[Goal]")
			{
				ValidateGoalLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[Player]")
			{
				ValidatePlayerLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[Checkpoints]")
			{
				ValidateCheckpointLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[Enemies]")
			{
				ValidateEnemyLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[MidBoss]")
			{
				std::istringstream values(line);
				std::string typeText;
				std::string surfaceText;
				std::string facingText;
				std::string hitPointText;
				std::string extra;
				float x = 0.0f;
				float y = 0.0f;
				int cameraGroupId = -1;
				int hitPoint = MidBossSpawnData::DefaultHitPoint;
				EnemySpawnSurface surface = EnemySpawnSurface::Floor;
				EnemySpawnFacing facing = EnemySpawnFacing::Left;
				const bool hasRequiredValues =
					static_cast<bool>(values >> typeText >> x >> y >>
						surfaceText >> facingText >> cameraGroupId);
				const bool hasHitPoint =
					static_cast<bool>(values >> hitPointText);
				const bool valid = hasRequiredValues &&
					typeText == "TurtleGrenadier" &&
					TryParseEnemySurface(surfaceText, surface) &&
					TryParseEnemyFacing(facingText, facing) &&
					cameraGroupId >= 0 &&
					(!hasHitPoint ||
						TryParseBossHitPoint(hitPointText, hitPoint)) &&
					!(values >> extra);
				if (!valid)
				{
					WriteStageWarning(stageNumber,
						"[MidBoss] line " + std::to_string(lineNumber) +
						" should be: TurtleGrenadier x y "
						"Floor|Ceiling Left|Right cameraGroupId [HP].");
				}
			}
			else if (currentSection == "[Boss]")
			{
				std::istringstream values(line);
				std::string typeText;
				std::string facingText;
				std::string hitPointText;
				std::string extra;
				float x = 0.0f;
				float y = 0.0f;
				int cameraGroupId = -1;
				int hitPoint = BossSpawnData::DefaultHitPoint;
				EnemySpawnFacing facing = EnemySpawnFacing::Left;
				const bool hasRequiredValues =
					static_cast<bool>(values >> typeText >> x >> y >>
						facingText >> cameraGroupId);
				const bool hasHitPoint =
					static_cast<bool>(values >> hitPointText);
				const bool valid = hasRequiredValues &&
					typeText == "Tank" &&
					TryParseEnemyFacing(facingText, facing) &&
					cameraGroupId >= 0 &&
					(!hasHitPoint ||
						TryParseBossHitPoint(hitPointText, hitPoint)) &&
					!(values >> extra);
				if (!valid)
				{
					WriteStageWarning(stageNumber,
						"[Boss] line " + std::to_string(lineNumber) +
						" should be: Tank x y Left|Right cameraGroupId [HP].");
				}
			}
			else if (currentSection == "[CameraAreas]")
			{
				ValidateCameraAreaLine(stageNumber, line, lineNumber);
			}
			else if (currentSection == "[TileSheets]" ||
				currentSection == "[VisualTiles]")
			{
				// 画像タイルはLoadVisualTilesで範囲と参照番号を確認する。
			}
			else
			{
				WriteStageWarning(
					stageNumber,
					"line " + std::to_string(lineNumber) +
					" is outside a known section.");
			}
		}

		if (tileRowCount <= 0)
		{
			WriteStageWarning(stageNumber, "tile map is empty.");
		}
	}
}

bool StageFileLoader::LoadVisualTiles(int stageNumber, StageData& stageData)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	if (!file.is_open())
	{
		return false;
	}

	stageData.tileSheetCount = 0;
	stageData.tileSheetNames = {};
	for (auto& row : stageData.solidVisuals)
	{
		row.fill(VisualTile{});
	}
	for (auto& row : stageData.backgroundVisuals)
	{
		row.fill(VisualTile{});
	}
	std::string line;
	std::string section;
	bool found = false;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (line.empty())
		{
			continue;
		}
		if (line.front() == '[')
		{
			section = line;
			continue;
		}
		if (section == "[TileSheets]")
		{
			if (stageData.tileSheetCount < StageData::MaxTileSheets &&
				line.find('/') == std::string::npos &&
				line.find('\\') == std::string::npos &&
				line.find("..") == std::string::npos)
			{
				stageData.tileSheetNames[stageData.tileSheetCount++] = line;
				found = true;
			}
		}
		else if (section == "[VisualTiles]")
		{
			std::istringstream stream(line);
			char layer = 0;
			int column = -1;
			int row = -1;
			int sheet = -1;
			int sourceColumn = -1;
			int sourceRow = -1;
			int frameCount = 1;
			int flipX = 0;
			int flipY = 0;
			if (!(stream >> layer >> column >> row >> sheet >>
				sourceColumn >> sourceRow) ||
				column < 0 || column >= StageData::TileColumns ||
				row < 0 || row >= stageData.tileRowCount ||
				sheet < 0 || sheet >= stageData.tileSheetCount ||
				sourceColumn < 0 || sourceColumn > 255 ||
				sourceRow < 0 || sourceRow > 255)
			{
				continue;
			}
			// 旧形式はコマ数や反転設定を持たないため、静止画・反転なしとして扱う。
			if (stream >> frameCount)
			{
				if (frameCount < 1 || frameCount > 255)
				{
					continue;
				}
				if (stream >> flipX)
				{
					if (!(stream >> flipY) ||
						(flipX != 0 && flipX != 1) ||
						(flipY != 0 && flipY != 1))
					{
						continue;
					}
				}
				else if (!stream.eof())
				{
					continue;
				}
			}
			else if (!stream.eof())
			{
				continue;
			}
			const VisualTile tile{
				static_cast<short>(sheet),
				static_cast<unsigned char>(sourceColumn),
				static_cast<unsigned char>(sourceRow),
				static_cast<unsigned char>(frameCount),
				flipX != 0,
				flipY != 0
			};
			if (layer == 'S' && StageData::IsSolidTile(stageData.tiles[row][column]))
			{
				stageData.solidVisuals[row][column] = tile;
			}
			else if (layer == 'B')
			{
				stageData.backgroundVisuals[row][column] = tile;
			}
		}
	}
	return found;
}

bool StageFileLoader::LoadTileMap(
	int stageNumber,
	std::vector<StageData::TileRow>& tiles,
	int& tileRowCount)
{
	if (tiles.size() != StageData::TileRows)
	{
		tiles.resize(StageData::TileRows);
	}

	std::vector<std::string> tileLines;

	if (!ReadTileLines(stageNumber, tileLines))
	{
		return false;
	}

	// ステージ制作中の入力ミスをログへ出す。
	// 読み込み自体は続けるので、多少のミスがあってもゲームは起動できる。
	ValidateStageFile(stageNumber);

	tileRowCount = static_cast<int>(tileLines.size());

	for (int y = 0; y < tileRowCount; ++y)
	{
		const std::string& line = tileLines[y];

		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			if (x < static_cast<int>(line.size()))
			{
				tiles[y][x] = NormalizeTile(line[x]);
			}
			else
			{
				tiles[y][x] = '.';
			}
		}
	}

	return true;
}

bool StageFileLoader::LoadFallHoles(
	int stageNumber,
	std::array<FallHole, StageData::MaxFallHoles>& fallHoles,
	int& fallHoleCount)
{
	const std::string filePath =
		BuildStageFilePath(stageNumber);
	std::ifstream file(filePath);

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundFallHoleSection = false;

	// [FallHoles] が出てくるまでは、タイル行や空行を読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[FallHoles]")
		{
			foundFallHoleSection = true;
			break;
		}
	}

	if (!foundFallHoleSection)
	{
		return false;
	}

	fallHoles = {};
	fallHoleCount = 0;
	std::vector<std::string> tileLines;
	ReadTileLines(stageNumber, tileLines);
	const int stageHeight = static_cast<int>(tileLines.size()) *
		StageData::TileSize;
	const int boundaryThickness = StageData::TileSize * 2;

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		int left = 0;
		int right = 0;
		int top = -1;
		int bottom = -1;

		if (!(lineStream >> left >> right))
		{
			continue;
		}

		const bool hasVerticalRange =
			static_cast<bool>(lineStream >> top >> bottom);
		if (hasVerticalRange)
		{
			if (fallHoleCount >= StageData::MaxFallHoles)
			{
				break;
			}
			fallHoles[fallHoleCount++] = { left, right, top, bottom };
			continue;
		}

		// 旧形式の「left right」は上下両方の外周穴だったため、
		// 読み込み時に天井穴と床穴の2つへ分ける。
		if (fallHoleCount < StageData::MaxFallHoles)
		{
			fallHoles[fallHoleCount++] = {
				left, right, 0, boundaryThickness
			};
		}
		if (stageHeight > 0 && fallHoleCount < StageData::MaxFallHoles)
		{
			fallHoles[fallHoleCount++] = {
				left, right, stageHeight - boundaryThickness, stageHeight
			};
		}
	}

	return true;
}

bool StageFileLoader::LoadTileFallHoles(
	int stageNumber,
	std::array<FallHole, StageData::MaxFallHoles>& fallHoles,
	int& fallHoleCount)
{
	std::vector<std::string> tileLines;

	fallHoles = {};
	fallHoleCount = 0;

	if (!ReadTileLines(stageNumber, tileLines) || tileLines.size() < 4)
	{
		return false;
	}

	// 旧ステージは天井と床を別々に調べ、片側だけの穴も認識する。
	const int stageHeight = static_cast<int>(tileLines.size()) *
		StageData::TileSize;
	const int boundaryThickness = StageData::TileSize * 2;
	auto scanBoundary = [&](const std::string& boundaryLine,
		int top, int bottom)
	{
		const int scanColumnCount = std::min(StageData::TileColumns,
			static_cast<int>(boundaryLine.size()));
		int x = 1;
		while (x < scanColumnCount - 1 &&
			fallHoleCount < StageData::MaxFallHoles)
		{
			if (IsSolidSourceTile(boundaryLine[x]))
			{
				++x;
				continue;
			}

			const int openingStartColumn = x;
			while (x < scanColumnCount - 1 &&
				!IsSolidSourceTile(boundaryLine[x]))
			{
				++x;
			}
			fallHoles[fallHoleCount++] = {
				openingStartColumn * StageData::TileSize,
				x * StageData::TileSize,
				top,
				bottom
			};
		}
	};

	scanBoundary(tileLines[1], 0, boundaryThickness);
	scanBoundary(tileLines[tileLines.size() - 2],
		stageHeight - boundaryThickness, stageHeight);

	return fallHoleCount > 0;
}

bool StageFileLoader::LoadGravityBlocks(
	int stageNumber,
	std::array<GravityBlockSpawnData, StageData::MaxGravityBlockSpawns>& gravityBlockSpawns,
	int& gravityBlockSpawnCount)
{
	const std::string filePath =
		BuildStageFilePath(stageNumber);
	std::ifstream file(filePath);

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundGravityBlockSection = false;

	// [GravityBlocks] が出てくるまでは、他のセクションを読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[GravityBlocks]")
		{
			foundGravityBlockSection = true;
			break;
		}
	}

	if (!foundGravityBlockSection)
	{
		return false;
	}

	gravityBlockSpawns = {};
	gravityBlockSpawnCount = 0;

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;
		std::string surfaceText;
		std::string motionText;

		if (!(lineStream >> x >> y))
		{
			continue;
		}

		if (gravityBlockSpawnCount >= StageData::MaxGravityBlockSpawns)
		{
			break;
		}

		GravityBlockSpawnData& spawnData =
			gravityBlockSpawns[gravityBlockSpawnCount];
		spawnData.position = { x, y };
		spawnData.surface = GravityBlockSpawnSurface::Floor;
		spawnData.motion = GravityBlockMotion::Gravity;

		// 3つ目の値は省略可能。省略時は従来どおり床側に置く。
		if (lineStream >> surfaceText && surfaceText == "Ceiling")
		{
			spawnData.surface = GravityBlockSpawnSurface::Ceiling;
		}

		if (lineStream >> motionText)
		{
			if (motionText == "Vertical")
			{
				spawnData.motion = GravityBlockMotion::Vertical;
			}
			else if (motionText == "Horizontal")
			{
				spawnData.motion = GravityBlockMotion::Horizontal;
			}
		}

		++gravityBlockSpawnCount;
	}

	return true;
}

bool StageFileLoader::LoadGoalPosition(
	int stageNumber,
	Vector2& goalPosition)
{
	const std::string filePath =
		BuildStageFilePath(stageNumber);
	std::ifstream file(filePath);

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundGoalSection = false;

	// [Goal] が出てくるまでは、他のセクションを読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[Goal]")
		{
			foundGoalSection = true;
			break;
		}
	}

	if (!foundGoalSection)
	{
		return false;
	}

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;

		if (!(lineStream >> x >> y))
		{
			return false;
		}

		goalPosition = { x, y };
		return true;
	}

	return false;
}

bool StageFileLoader::LoadPlayerStartPosition(
	int stageNumber,
	Vector2& playerStartPosition)
{
	const std::string filePath =
		BuildStageFilePath(stageNumber);
	std::ifstream file(filePath);

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundPlayerSection = false;

	// [Player] が出てくるまでは、他のセクションを読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[Player]")
		{
			foundPlayerSection = true;
			break;
		}
	}

	if (!foundPlayerSection)
	{
		return false;
	}

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		float x = 0.0f;
		float y = 0.0f;

		if (!(lineStream >> x >> y))
		{
			return false;
		}

		playerStartPosition = { x, y };
		return true;
	}

	return false;
}

bool StageFileLoader::LoadTileObjectPositions(
	int stageNumber,
	Vector2& playerStartPosition,
	bool& hasPlayerStartPosition,
	Vector2& goalPosition,
	bool& hasGoalPosition)
{
	std::vector<std::string> tileLines;

	hasPlayerStartPosition = false;
	hasGoalPosition = false;

	if (!ReadTileLines(stageNumber, tileLines))
	{
		return false;
	}

	for (int y = 0; y < static_cast<int>(tileLines.size()); ++y)
	{
		const std::string& line = tileLines[y];

		for (int x = 0; x < static_cast<int>(line.size()) &&
			x < StageData::TileColumns; ++x)
		{
			const Vector2 objectPosition{
				static_cast<float>(x * StageData::TileSize),
				static_cast<float>(y * StageData::TileSize)
			};

			if (line[x] == StageData::PlayerStartTile)
			{
				playerStartPosition = objectPosition;
				hasPlayerStartPosition = true;
			}
			else if (line[x] == StageData::GoalTile)
			{
				goalPosition = objectPosition;
				hasGoalPosition = true;
			}
		}
	}

	return hasPlayerStartPosition || hasGoalPosition;
}

bool StageFileLoader::LoadCheckpoints(
	int stageNumber,
	std::array<Vector2, StageData::MaxCheckpoints>& checkpoints,
	int& checkpointCount)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundSection = false;
	while (std::getline(file, line))
	{
		if (line == "[Checkpoints]")
		{
			foundSection = true;
			break;
		}
	}

	if (!foundSection)
	{
		return false;
	}

	checkpoints = {};
	checkpointCount = 0;
	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}
		if (line[0] == '[')
		{
			break;
		}

		std::istringstream values(line);
		float x = 0.0f;
		float y = 0.0f;
		if (!(values >> x >> y))
		{
			continue;
		}
		if (checkpointCount >= StageData::MaxCheckpoints)
		{
			break;
		}

		checkpoints[checkpointCount++] = { x, y };
	}

	return true;
}

bool StageFileLoader::LoadTileCheckpoints(
	int stageNumber,
	std::array<Vector2, StageData::MaxCheckpoints>& checkpoints,
	int& checkpointCount)
{
	std::vector<std::string> tileLines;
	checkpoints = {};
	checkpointCount = 0;
	if (!ReadTileLines(stageNumber, tileLines))
	{
		return false;
	}

	for (int y = 0; y < static_cast<int>(tileLines.size()); ++y)
	{
		for (int x = 0;
			x < static_cast<int>(tileLines[y].size()) &&
			x < StageData::TileColumns;
			++x)
		{
			if (tileLines[y][x] != StageData::CheckpointTile)
			{
				continue;
			}
			if (checkpointCount >= StageData::MaxCheckpoints)
			{
				return true;
			}

			checkpoints[checkpointCount++] = {
				static_cast<float>(x * StageData::TileSize),
				static_cast<float>(y * StageData::TileSize)
			};
		}
	}

	return checkpointCount > 0;
}

bool StageFileLoader::LoadTileGravityBlocks(
	int stageNumber,
	std::array<GravityBlockSpawnData, StageData::MaxGravityBlockSpawns>& gravityBlockSpawns,
	int& gravityBlockSpawnCount)
{
	std::vector<std::string> tileLines;

	gravityBlockSpawns = {};
	gravityBlockSpawnCount = 0;

	if (!ReadTileLines(stageNumber, tileLines))
	{
		return false;
	}

	for (int y = 0; y < static_cast<int>(tileLines.size()); ++y)
	{
		const std::string& line = tileLines[y];

		for (int x = 0; x < static_cast<int>(line.size()) &&
			x < StageData::TileColumns; ++x)
		{
			if (line[x] != StageData::GravityBlockTile)
			{
				continue;
			}

			if (gravityBlockSpawnCount >= StageData::MaxGravityBlockSpawns)
			{
				return true;
			}

			// 64x64のブロックを、40x40の記号マスの中央へ合わせる。
			constexpr float blockSize = 64.0f;
			const bool isCeiling = IsCeilingObjectMarker(tileLines, x, y);
			const float cellLeft = static_cast<float>(x * StageData::TileSize);
			const float cellTop = static_cast<float>(y * StageData::TileSize);
			const float cellBottom = cellTop + StageData::TileSize;

			GravityBlockSpawnData& spawnData =
				gravityBlockSpawns[gravityBlockSpawnCount];
			spawnData.position = {
				cellLeft + (StageData::TileSize - blockSize) * 0.5f,
				isCeiling ? cellTop : cellBottom - blockSize
			};
			spawnData.surface =
				isCeiling
				? GravityBlockSpawnSurface::Ceiling
				: GravityBlockSpawnSurface::Floor;

			++gravityBlockSpawnCount;
		}
	}

	return gravityBlockSpawnCount > 0;
}

bool StageFileLoader::LoadTileEnemies(
	int stageNumber,
	std::array<EnemySpawnData, StageData::MaxEnemySpawns>& enemySpawns,
	int& enemySpawnCount)
{
	std::vector<std::string> tileLines;

	enemySpawns = {};
	enemySpawnCount = 0;

	if (!ReadTileLines(stageNumber, tileLines))
	{
		return false;
	}

	for (int y = 0; y < static_cast<int>(tileLines.size()); ++y)
	{
		const std::string& line = tileLines[y];

		for (int x = 0; x < static_cast<int>(line.size()) &&
			x < StageData::TileColumns; ++x)
		{
			const char tile = line[x];

			if (tile != StageData::NormalEnemyTile &&
				tile != StageData::HeavyEnemyTile &&
				tile != StageData::ShooterEnemyTile &&
				tile != StageData::UpShooterEnemyTile &&
				tile != StageData::ChargerEnemyTile &&
				tile != StageData::MortarEnemyTile)
			{
				continue;
			}

			if (enemySpawnCount >= StageData::MaxEnemySpawns)
			{
				return true;
			}

			// 上だけが地形なら天井側、それ以外は床側として扱う。
			const EnemySpawnSurface surface =
				IsCeilingObjectMarker(tileLines, x, y)
				? EnemySpawnSurface::Ceiling
				: EnemySpawnSurface::Floor;

			EnemyType enemyType = EnemyType::Normal;
			float enemyWidth = 48.0f;
			float enemyHeight = 48.0f;
			int hitPoint = 1;

			if (tile == StageData::HeavyEnemyTile)
			{
				enemyType = EnemyType::Heavy;
				enemyWidth = 56.0f;
				enemyHeight = 56.0f;
				hitPoint = 2;
			}
			else if (tile == StageData::ShooterEnemyTile)
			{
				enemyType = EnemyType::Shooter;
			}
			else if (tile == StageData::UpShooterEnemyTile)
			{
				enemyType = EnemyType::UpShooter;
			}
			else if (tile == StageData::ChargerEnemyTile)
			{
				enemyType = EnemyType::Charger;
				enemyWidth = 80.0f;
				enemyHeight = 80.0f;
			}
			else if (tile == StageData::MortarEnemyTile)
			{
				enemyType = EnemyType::Mortar;
				enemyWidth = 60.0f;
				enemyHeight = 60.0f;
			}

			const float cellLeft = static_cast<float>(x * StageData::TileSize);
			const float cellTop = static_cast<float>(y * StageData::TileSize);

			EnemySpawnData& spawnData = enemySpawns[enemySpawnCount];
			spawnData.position.x =
				cellLeft + (StageData::TileSize - enemyWidth) * 0.5f;
			spawnData.position.y =
				surface == EnemySpawnSurface::Ceiling
				? cellTop
				: cellTop + StageData::TileSize - enemyHeight;
			spawnData.hitPoint = hitPoint;
			spawnData.surface = surface;
			spawnData.type = enemyType;
			if (enemyType == EnemyType::Charger ||
				enemyType == EnemyType::Mortar)
			{
				spawnData.isAffectedByGravityReversal = false;
			}

			++enemySpawnCount;
		}
	}

	return enemySpawnCount > 0;
}

bool StageFileLoader::LoadEnemies(
	int stageNumber,
	std::array<EnemySpawnData, StageData::MaxEnemySpawns>& enemySpawns,
	int& enemySpawnCount)
{
	const std::string filePath =
		BuildStageFilePath(stageNumber);
	std::ifstream file(filePath);

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	bool foundEnemySection = false;

	// [Enemies] が出てくるまでは、他のセクションを読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[Enemies]")
		{
			foundEnemySection = true;
			break;
		}
	}

	if (!foundEnemySection)
	{
		return false;
	}

	enemySpawns = {};
	enemySpawnCount = 0;

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		std::string typeText;
		std::string surfaceText;
		std::string gravityResponseText;
		std::string facingText;
		float x = 0.0f;
		float y = 0.0f;
		int hitPoint = 1;

		if (!(lineStream >> typeText >> x >> y >> hitPoint >> surfaceText))
		{
			continue;
		}

		if (enemySpawnCount >= StageData::MaxEnemySpawns)
		{
			break;
		}

		EnemyType type = EnemyType::Normal;
		EnemySpawnSurface surface = EnemySpawnSurface::Floor;
		bool isAffectedByGravityReversal = true;
		EnemySpawnFacing facing = EnemySpawnFacing::Left;

		if (!TryParseEnemyType(typeText, type) ||
			!TryParseEnemySurface(surfaceText, surface))
		{
			continue;
		}

		if (lineStream >> gravityResponseText &&
			!TryParseEnemyGravityResponse(
				gravityResponseText, isAffectedByGravityReversal))
		{
			continue;
		}

		if (lineStream >> facingText && !TryParseEnemyFacing(facingText, facing))
		{
			continue;
		}
		if (type == EnemyType::Charger || type == EnemyType::Mortar)
		{
			isAffectedByGravityReversal = false;
		}

		EnemySpawnData& spawnData = enemySpawns[enemySpawnCount];
		spawnData.position = { x, y };
		spawnData.hitPoint = hitPoint;
		spawnData.surface = surface;
		spawnData.type = type;
		spawnData.isAffectedByGravityReversal =
			isAffectedByGravityReversal;
		spawnData.facing = facing;

		++enemySpawnCount;
	}

	return true;
}

bool StageFileLoader::LoadMidBoss(
	int stageNumber,
	MidBossSpawnData& midBossSpawn,
	bool& hasMidBossSpawn)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	hasMidBossSpawn = false;
	midBossSpawn = {};

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	while (std::getline(file, line) && line != "[MidBoss]")
	{
	}
	if (line != "[MidBoss]")
	{
		return false;
	}

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}
		if (line.front() == '[')
		{
			break;
		}

		std::istringstream stream(line);
		std::string typeText;
		std::string surfaceText;
		std::string facingText;
		std::string hitPointText;
		std::string extra;
		float x = 0.0f;
		float y = 0.0f;
		int cameraGroupId = -1;
		if (!(stream >> typeText >> x >> y >> surfaceText >> facingText >> cameraGroupId))
		{
			continue;
		}

		EnemySpawnSurface surface = EnemySpawnSurface::Floor;
		EnemySpawnFacing facing = EnemySpawnFacing::Left;
		int hitPoint = MidBossSpawnData::DefaultHitPoint;
		if (typeText != "TurtleGrenadier" ||
			!TryParseEnemySurface(surfaceText, surface) ||
			!TryParseEnemyFacing(facingText, facing) ||
			cameraGroupId < 0)
		{
			continue;
		}
		if (stream >> hitPointText)
		{
			if (!TryParseBossHitPoint(hitPointText, hitPoint) || stream >> extra)
			{
				continue;
			}
		}

		midBossSpawn.position = { x, y };
		midBossSpawn.hitPoint = hitPoint;
		midBossSpawn.surface = surface;
		midBossSpawn.facing = facing;
		midBossSpawn.cameraGroupId = cameraGroupId;
		hasMidBossSpawn = true;
		return true;
	}

	return true;
}

bool StageFileLoader::LoadBoss(
	int stageNumber,
	BossSpawnData& bossSpawn,
	bool& hasBossSpawn)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	hasBossSpawn = false;
	bossSpawn = {};

	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	while (std::getline(file, line) && line != "[Boss]")
	{
	}
	if (line != "[Boss]")
	{
		return false;
	}

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}
		if (line.front() == '[')
		{
			break;
		}

		std::istringstream stream(line);
		std::string typeText;
		std::string facingText;
		std::string hitPointText;
		std::string extra;
		float x = 0.0f;
		float y = 0.0f;
		int cameraGroupId = -1;
		if (!(stream >> typeText >> x >> y >> facingText >> cameraGroupId))
		{
			continue;
		}

		EnemySpawnFacing facing = EnemySpawnFacing::Left;
		int hitPoint = BossSpawnData::DefaultHitPoint;
		if (typeText != "Tank" ||
			!TryParseEnemyFacing(facingText, facing) || cameraGroupId < 0)
		{
			continue;
		}
		if (stream >> hitPointText)
		{
			if (!TryParseBossHitPoint(hitPointText, hitPoint) || stream >> extra)
			{
				continue;
			}
		}

		bossSpawn.position = { x, y };
		bossSpawn.hitPoint = hitPoint;
		bossSpawn.facing = facing;
		bossSpawn.cameraGroupId = cameraGroupId;
		hasBossSpawn = true;
		return true;
	}

	return true;
}

bool StageFileLoader::LoadSaws(int stageNumber,
	std::vector<SawSpawnData>& sawSpawns,
	int& sawSpawnCount)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	if (!file.is_open())
	{
		return false;
	}

	std::string line;
	while (std::getline(file, line) && line != "[Saws]")
	{
	}
	if (line != "[Saws]")
	{
		return false;
	}

	sawSpawns.clear();
	sawSpawnCount = 0;
	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}
		if (line.front() == '[')
		{
			break;
		}
		std::istringstream values(line);
		SawSpawnData saw{};
		int flipX = 0;
		int flipY = 0;
		std::string extra;
		if (!(values >> saw.position.x >> saw.position.y))
		{
			continue;
		}
		if (values >> flipX)
		{
			if (!(values >> flipY) || (values >> extra) ||
				(flipX != 0 && flipX != 1) || (flipY != 0 && flipY != 1))
			{
				continue;
			}
		}
		else if (!values.eof())
		{
			continue;
		}
		if (saw.position.x >= 0.0f &&
			saw.position.x < StageData::StagePixelWidth &&
			saw.position.y >= 0.0f &&
			saw.position.y < StageData::MaxStagePixelHeight)
		{
			saw.flipX = flipX != 0;
			saw.flipY = flipY != 0;
			sawSpawns.push_back(saw);
			sawSpawnCount = static_cast<int>(sawSpawns.size());
		}
	}
	return true;
}

bool StageFileLoader::LoadCameraAreas(
	int stageNumber,
	std::array<CameraArea, StageData::MaxCameraAreas>& cameraAreas,
	int& cameraAreaCount)
{
	std::ifstream file(BuildStageFilePath(stageNumber));
	std::string line;
	bool foundCameraAreaSection = false;

	if (!file.is_open())
	{
		return false;
	}

	// [CameraAreas] が出てくるまでは、タイル行や他のセクションを読み飛ばす。
	while (std::getline(file, line))
	{
		if (line == "[CameraAreas]")
		{
			foundCameraAreaSection = true;
			break;
		}
	}

	if (!foundCameraAreaSection)
	{
		return false;
	}

	cameraAreas = {};
	cameraAreaCount = 0;

	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '[')
		{
			break;
		}

		std::istringstream lineStream(line);
		std::string modeText;
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		int groupId = -1;

		if (!(lineStream >> modeText >> left >> top >> right >> bottom))
		{
			continue;
		}
		if (!(lineStream >> groupId))
		{
			groupId = cameraAreaCount;
		}

		CameraAreaMode mode = CameraAreaMode::Free;

		if (!TryParseCameraAreaMode(modeText, mode) || groupId < 0 ||
			right <= left || bottom <= top)
		{
			continue;
		}

		if (cameraAreaCount >= StageData::MaxCameraAreas)
		{
			break;
		}

		CameraArea& area = cameraAreas[cameraAreaCount];
		area.mode = mode;
		area.left = left;
		area.top = top;
		area.right = right;
		area.bottom = bottom;
		area.groupId = groupId;

		++cameraAreaCount;
	}

	return cameraAreaCount > 0;
}

char StageFileLoader::NormalizeTile(char tile)
{
	if (tile == StageData::BlockTile ||
		tile == StageData::FloorTile ||
		tile == StageData::WallTile ||
		tile == StageData::InvisibleBlockTile)
	{
		return tile;
	}

	return StageData::EmptyTile;
}
