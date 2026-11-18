#include "StageManager.h"
#include "GameConfig.h"
#include "StageFileLoader.h"

#include <algorithm>
#include <cstring>

// 最初のステージをステージ1として開始。
void StageManager::Initialize()
{
	isGameCleared_ = false;
	SetupStage(1);
}

void StageManager::StartStage(int stageNumber)
{
	isGameCleared_ = false;

	if (stageNumber < 1)
	{
		stageNumber = 1;
	}

	if (stageNumber > LastStageNumber)
	{
		stageNumber = LastStageNumber;
	}

	SetupStage(stageNumber);
}

void StageManager::StartPreview(const StageData& stageData)
{
	isGameCleared_ = false;
	currentStage_ = stageData;
}

void StageManager::GoToNextStage()
{
	if (isGameCleared_)
	{
		return;
	}

	if (currentStage_.stageNumber >= LastStageNumber)
	{
		isGameCleared_ = true;
		return;
	}

	const int nextStageNumber = currentStage_.stageNumber + 1;

	SetupStage(nextStageNumber);
}

// 指定されたステージ番号に合わせて、ステージ情報を作り直す。
void StageManager::SetupStage(int stageNumber)
{
	currentStage_ = {};
	currentStage_.stageNumber = stageNumber;

	SetupClearCondition();
	SetupTileMap();
	StageFileLoader::LoadVisualTiles(stageNumber, currentStage_);
	SetupCameraAreas();
	SetupTileObjectPositions();
	SetupFallHoles();
	SetupGravityBlocks();
	SetupGoalPosition();
	SetupPlayerStartPosition();
	SetupCheckpoints();
	SetupEnemies();
	SetupMidBoss();
	SetupBoss();
	StageFileLoader::LoadSaws(stageNumber, currentStage_.sawSpawns,
		currentStage_.sawSpawnCount);
}

// ステージ番号に合わせて、クリア条件を設定する。
void StageManager::SetupClearCondition()
{
	if (currentStage_.stageNumber <= 3)
	{
		currentStage_.clearConditionType =
			ClearConditionType::ReachGoal;
	}
	else if (currentStage_.stageNumber == 4)
	{
		currentStage_.clearConditionType =
			ClearConditionType::DefeatMidBoss;
	}
	else
	{
		currentStage_.clearConditionType =
			ClearConditionType::DefeatBoss;
	}
}

// ステージ番号に合わせて、タイルマップの見た目を設定する。
void StageManager::SetupTileMap()
{
	ClearTileMap();

	// 外部ステージファイルがある場合は、そちらを優先して読み込む。
	// ファイルが無い場合は、この下の仮ステージ定義を使う。
	if (StageFileLoader::LoadTileMap(
		currentStage_.stageNumber,
		currentStage_.tiles,
		currentStage_.tileRowCount))
	{
		return;
	}

	switch (currentStage_.stageNumber)
	{
	case 1:
		SetTileMap({
			"################################",
			"################################",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#.................######.......#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"###################....#########",
			"###################....#########"
		});
		break;

	case 2:
		SetTileMap({
			"################################",
			"################################",
			"#..............................#",
			"#.................#####........#",
			"#..............................#",
			"#..............................#",
			"#......#####...................#",
			"#..............................#",
			"#..............................#",
			"#......................#####...#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"###########...########.....#####",
			"###########...########.....#####"
		});
		break;

	case 3:
		SetTileMap({
			"################################",
			"#######....#####....######...###",
			"#..............................#",
			"#..............................#",
			"#.......................#####..#",
			"#..............................#",
			"#.........#####................#",
			"#..............................#",
			"#..............................#",
			"#................#####.........#",
			"#..............................#",
			"#..............................#",
			"#.....#####....................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#######....#####....######...###",
			"#######....#####....######...###"
		});
		break;

	case 4:
		SetTileMap({
			"################################",
			"################################",
			"#..............................#",
			"#..............................#",
			"#...............#####..........#",
			"#..............................#",
			"#..............................#",
			"#......#####...................#",
			"#..............................#",
			"#..............................#",
			"#......................#####...#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#############....######....#####",
			"#############....######....#####"
		});
		break;

	case 5:
		SetTileMap({
			"################################",
			"##########....#####....###....##",
			"#..............................#",
			"#.........................####.#",
			"#..............................#",
			"#.......#####..................#",
			"#..............................#",
			"#................#####.........#",
			"#..............................#",
			"#..............................#",
			"#.....#####..............#####.#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"#..............................#",
			"##########....#####....###....##",
			"##########....#####....###....##"
		});
		break;

	default:
		break;
	}
}

// ステージ番号に合わせて、落下穴の配置を設定する。
// 穴は左から右の順番で追加しておくと、描画処理が正しく区間分けできる。
void StageManager::SetupFallHoles()
{
	ClearFallHoles();

	// 外部ステージファイルに穴情報がある場合は、そちらを優先して使う。
	// まだ穴情報が書かれていないステージでは、この下の仮設定を使う。
	if (StageFileLoader::LoadFallHoles(
		currentStage_.stageNumber,
		currentStage_.fallHoles,
		currentStage_.fallHoleCount))
	{
		return;
	}

	// 座標セクションが無ければ、上下の外周タイルにある共通の隙間を穴にする。
	if (StageFileLoader::LoadTileFallHoles(
		currentStage_.stageNumber,
		currentStage_.fallHoles,
		currentStage_.fallHoleCount))
	{
		return;
	}

	switch (currentStage_.stageNumber)
	{
	case 1:
		// ステージ1: 穴ループの練習用。穴は1つだけにして、動きを覚えやすくする。
		AddFallHole(760, 920);
		break;

	case 2:
		// ステージ2: 穴を2つに増やし、移動ルートを選ばせる。
		AddFallHole(440, 560);
		AddFallHole(880, 1080);
		break;

	case 3:
		// ステージ3: 床と天井を行き来しながら進む応用ステージ。
		AddFallHole(280, 440);
		AddFallHole(640, 800);
		AddFallHole(1040, 1160);
		break;

	case 4:
		// ステージ4: 中ボス前の道中。ブロックを盾にしやすい位置へ穴を置く。
		AddFallHole(520, 680);
		AddFallHole(920, 1080);
		break;

	case 5:
		// ステージ5: 大ボス前。穴とブロックを多めにして最終ステージらしくする。
		AddFallHole(400, 560);
		AddFallHole(760, 920);
		AddFallHole(1040, 1200);
		break;

	default:
		break;
	}
}

// ステージ番号に合わせて、重力ブロックの初期配置を設定する。
void StageManager::SetupGravityBlocks()
{
	ClearGravityBlocks();

	// 外部ステージファイルに重力ブロック配置がある場合は、そちらを優先して使う。
	// まだ配置が書かれていないステージでは、この下の仮設定を使う。
	if (StageFileLoader::LoadGravityBlocks(
		currentStage_.stageNumber,
		currentStage_.gravityBlockSpawns,
		currentStage_.gravityBlockSpawnCount))
	{
		return;
	}

	// 座標セクションが無ければ、タイルマップ上のBを使う。
	if (StageFileLoader::LoadTileGravityBlocks(
		currentStage_.stageNumber,
		currentStage_.gravityBlockSpawns,
		currentStage_.gravityBlockSpawnCount))
	{
		return;
	}

	switch (currentStage_.stageNumber)
	{
	case 1:
		// ステージ1: 敵弾を防ぐ練習用のブロックを1つだけ置く。
		AddGravityBlock(610.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		break;

	case 2:
		// ステージ2: 床側と天井側に分け、重力反転の使い分けを作る。
		AddGravityBlock(350.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(720.0f, static_cast<float>(GameConfig::CeilingY), GravityBlockSpawnSurface::Ceiling);
		AddGravityBlock(1030.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		break;

	case 3:
		// ステージ3: 連続して重力反転させるルートを作る。
		AddGravityBlock(440.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(720.0f, static_cast<float>(GameConfig::CeilingY), GravityBlockSpawnSurface::Ceiling);
		AddGravityBlock(980.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		break;

	case 4:
		// ステージ4: 射撃敵の弾を防ぎながら中ボスへ向かう。
		AddGravityBlock(480.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(760.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(1040.0f, static_cast<float>(GameConfig::CeilingY), GravityBlockSpawnSurface::Ceiling);
		break;

	case 5:
		// ステージ5: 複数の盾と穴を組み合わせた最終道中。
		AddGravityBlock(360.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(620.0f, static_cast<float>(GameConfig::CeilingY), GravityBlockSpawnSurface::Ceiling);
		AddGravityBlock(920.0f, static_cast<float>(GameConfig::FloorY) - 64.0f);
		AddGravityBlock(1110.0f, static_cast<float>(GameConfig::CeilingY), GravityBlockSpawnSurface::Ceiling);
		break;

	default:
		break;
	}
}

// タイルマップ内のP/G記号から、プレイヤー開始位置とゴール位置を登録する。
void StageManager::SetupTileObjectPositions()
{
	StageFileLoader::LoadTileObjectPositions(
		currentStage_.stageNumber,
		currentStage_.playerStartPosition,
		currentStage_.hasPlayerStartPosition,
		currentStage_.goalPosition,
		currentStage_.hasGoalPosition);
}

// ステージファイルにゴール位置が書かれていれば、その位置を現在のステージへ登録する。
void StageManager::SetupGoalPosition()
{
	Vector2 goalPosition{};

	if (StageFileLoader::LoadGoalPosition(
		currentStage_.stageNumber,
		goalPosition))
	{
		currentStage_.goalPosition = goalPosition;
		currentStage_.hasGoalPosition = true;
	}
}

// ステージファイルにプレイヤー開始位置が書かれていれば、現在のステージへ登録する。
void StageManager::SetupPlayerStartPosition()
{
	Vector2 playerStartPosition{};

	if (StageFileLoader::LoadPlayerStartPosition(
		currentStage_.stageNumber,
		playerStartPosition))
	{
		currentStage_.playerStartPosition = playerStartPosition;
		currentStage_.hasPlayerStartPosition = true;
	}
}

// ステージファイルに敵配置が書かれていれば、その配置を現在のステージへ登録する。
void StageManager::SetupEnemies()
{
	ClearEnemies();

	// 細かな座標を指定する[Enemies]があれば、そちらを優先する。
	if (StageFileLoader::LoadEnemies(
		currentStage_.stageNumber,
		currentStage_.enemySpawns,
		currentStage_.enemySpawnCount))
	{
		return;
	}

	// [Enemies]が無ければ、タイルマップ上のE/Sを使う。
	StageFileLoader::LoadTileEnemies(
		currentStage_.stageNumber,
		currentStage_.enemySpawns,
		currentStage_.enemySpawnCount);
}

void StageManager::SetupMidBoss()
{
	currentStage_.midBossSpawn = {};
	currentStage_.hasMidBossSpawn = false;
	StageFileLoader::LoadMidBoss(
		currentStage_.stageNumber,
		currentStage_.midBossSpawn,
		currentStage_.hasMidBossSpawn);
}

void StageManager::SetupBoss()
{
	currentStage_.bossSpawn = {};
	currentStage_.hasBossSpawn = false;
	StageFileLoader::LoadBoss(
		currentStage_.stageNumber,
		currentStage_.bossSpawn,
		currentStage_.hasBossSpawn);
}

void StageManager::SetupCheckpoints()
{
	currentStage_.checkpoints = {};
	currentStage_.checkpointCount = 0;

	if (StageFileLoader::LoadCheckpoints(
		currentStage_.stageNumber,
		currentStage_.checkpoints,
		currentStage_.checkpointCount))
	{
		return;
	}

	StageFileLoader::LoadTileCheckpoints(
		currentStage_.stageNumber,
		currentStage_.checkpoints,
		currentStage_.checkpointCount);
}

// ステージファイルのカメラエリアを読み込む。
// 設定が無いステージでは、ステージ全体を自由追従エリアとして扱う。
void StageManager::SetupCameraAreas()
{
	ClearCameraAreas();

	if (StageFileLoader::LoadCameraAreas(
		currentStage_.stageNumber,
		currentStage_.cameraAreas,
		currentStage_.cameraAreaCount))
	{
		return;
	}

	AddCameraArea(
		CameraAreaMode::Free,
		0,
		0,
		StageData::StagePixelWidth,
		currentStage_.GetPixelHeight());
}

// 登録済みの落下穴をすべて消す。
void StageManager::ClearFallHoles()
{
	currentStage_.fallHoles = {};
	currentStage_.fallHoleCount = 0;
}

// タイルマップを空間で初期化する。
void StageManager::ClearTileMap()
{
	currentStage_.tileRowCount = StageData::DefaultTileRows;

	for (int y = 0; y < StageData::TileRows; ++y)
	{
		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			currentStage_.tiles[y][x] = StageData::EmptyTile;
		}
	}
}

// 登録済みの重力ブロック配置をすべて消す。
void StageManager::ClearGravityBlocks()
{
	currentStage_.gravityBlockSpawns = {};
	currentStage_.gravityBlockSpawnCount = 0;
}

// 登録済みの敵配置をすべて消す。
void StageManager::ClearEnemies()
{
	currentStage_.enemySpawns = {};
	currentStage_.enemySpawnCount = 0;
}

// 文字列で書いたステージ形状を、StageDataのタイル配列へコピーする。
void StageManager::SetTileMap(
	std::initializer_list<const char*> rows)
{
	currentStage_.tileRowCount = std::min(
		static_cast<int>(rows.size()),
		StageData::TileRows);

	int y = 0;

	for (const char* row : rows)
	{
		if (y >= currentStage_.tileRowCount)
		{
			break;
		}

		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			const int rowLength =
				static_cast<int>(std::strlen(row));
			char tile = StageData::EmptyTile;

			if (x < rowLength)
			{
				tile = row[x];
			}

			// 既存の'#'だけで書いたマップを、描画用のタイル種類へ自動分類する。
			// 上下の床や中段の横足場は'='、左右の壁は'|'として扱う。
			if (tile == StageData::BlockTile)
			{
				if (x == 0 || x == StageData::TileColumns - 1)
				{
					tile = StageData::WallTile;
				}
				else
				{
					tile = StageData::FloorTile;
				}
			}

			currentStage_.tiles[y][x] = tile;
		}

		++y;
	}
}

// 登録済みのカメラエリアをすべて消す。
void StageManager::ClearCameraAreas()
{
	currentStage_.cameraAreas = {};
	currentStage_.cameraAreaCount = 0;
}

// 落下穴を1つ追加する。
// 上限数を超えた場合は、配列の外へ書き込まないように何もしない。
void StageManager::AddFallHole(int left, int right)
{
	if (currentStage_.fallHoleCount >= StageData::MaxFallHoles)
	{
		return;
	}

	currentStage_.fallHoles[currentStage_.fallHoleCount++] =
		{ left, right };
}

// 重力ブロックの初期配置を1つ追加する。
// 上限数を超えた場合は、配列の外へ書き込まないように何もしない。
void StageManager::AddGravityBlock(
	float x,
	float y,
	GravityBlockSpawnSurface surface)
{
	if (currentStage_.gravityBlockSpawnCount >=
		StageData::MaxGravityBlockSpawns)
	{
		return;
	}

	GravityBlockSpawnData& spawnData =
		currentStage_.gravityBlockSpawns[currentStage_.gravityBlockSpawnCount++];
	spawnData.position = { x, y };
	spawnData.surface = surface;
}

// カメラエリアを1つ追加する。
void StageManager::AddCameraArea(
	CameraAreaMode mode,
	int left,
	int top,
	int right,
	int bottom)
{
	if (currentStage_.cameraAreaCount >= StageData::MaxCameraAreas ||
		right <= left || bottom <= top)
	{
		return;
	}

	CameraArea& area =
		currentStage_.cameraAreas[currentStage_.cameraAreaCount];
	area.mode = mode;
	area.left = left;
	area.top = top;
	area.right = right;
	area.bottom = bottom;
	area.groupId = currentStage_.cameraAreaCount;

	++currentStage_.cameraAreaCount;
}

// 現在のステージ情報を外部から参照できるようにする。
const StageData& StageManager::GetCurrentStage() const
{
	return currentStage_;
}

bool StageManager::IsGameCleared() const
{
	return isGameCleared_;
}
