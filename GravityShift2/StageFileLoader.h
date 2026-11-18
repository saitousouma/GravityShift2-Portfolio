#pragma once

#include "StageData.h"

// 外部テキストファイルからステージのタイル配置を読み込むクラス。
// 将来ステージエディターを作ったとき、この形式で保存すればそのままゲームに使える。
class StageFileLoader
{
public:
	// 地形とは別に保存した画像タイルを読み込む。古いファイルでは何もしない。
	static bool LoadVisualTiles(int stageNumber, StageData& stageData);
	// stageNumberに対応する Assets/Stages/stageN.txt を読み込む。
	// 読み込みに成功したらtrue、ファイルが無い・読めない場合はfalseを返す。
	static bool LoadTileMap(
		int stageNumber,
		std::vector<StageData::TileRow>& tiles,
		int& tileRowCount
	);

	// stageNumberに対応する Assets/Stages/stageN.txt から穴情報を読み込む。
	// [FallHoles] の left right [top bottom] をFallHole配列へ入れる。
	static bool LoadFallHoles(
		int stageNumber,
		std::array<FallHole, StageData::MaxFallHoles>& fallHoles,
		int& fallHoleCount
	);

	// タイルマップの上端と下端にある同じ範囲の隙間を、落下穴として読み込む。
	static bool LoadTileFallHoles(
		int stageNumber,
		std::array<FallHole, StageData::MaxFallHoles>& fallHoles,
		int& fallHoleCount
	);

	// stageNumberに対応する Assets/Stages/stageN.txt から重力ブロック配置を読み込む。
	// [GravityBlocks] の下にある x y の組を、重力ブロック配置配列へ入れる。
	static bool LoadGravityBlocks(
		int stageNumber,
		std::array<GravityBlockSpawnData, StageData::MaxGravityBlockSpawns>& gravityBlockSpawns,
		int& gravityBlockSpawnCount
	);

	// stageNumberに対応する Assets/Stages/stageN.txt からゴール位置を読み込む。
	// [Goal] の下にある x y を、ゴールの左上座標として扱う。
	static bool LoadGoalPosition(
		int stageNumber,
		Vector2& goalPosition
	);

	// stageNumberに対応する Assets/Stages/stageN.txt からプレイヤー開始位置を読み込む。
	// [Player] の下にある x y を、プレイヤーの左上座標として扱う。
	static bool LoadPlayerStartPosition(
		int stageNumber,
		Vector2& playerStartPosition
	);

	static bool LoadCheckpoints(
		int stageNumber,
		std::array<Vector2, StageData::MaxCheckpoints>& checkpoints,
		int& checkpointCount
	);

	static bool LoadTileCheckpoints(
		int stageNumber,
		std::array<Vector2, StageData::MaxCheckpoints>& checkpoints,
		int& checkpointCount
	);

	// タイルマップ上の P / G から、プレイヤー開始位置とゴール位置を読み込む。
	// 見つからなかった場合は、それぞれのhas...にfalseを入れる。
	static bool LoadTileObjectPositions(
		int stageNumber,
		Vector2& playerStartPosition,
		bool& hasPlayerStartPosition,
		Vector2& goalPosition,
		bool& hasGoalPosition
	);

	// タイルマップ上のBから重力ブロックの配置を読み込む。
	// Bを置いたマスの下辺に、ブロックの下辺が合うように配置する。
	static bool LoadTileGravityBlocks(
		int stageNumber,
		std::array<GravityBlockSpawnData, StageData::MaxGravityBlockSpawns>& gravityBlockSpawns,
		int& gravityBlockSpawnCount
	);

	// タイルマップ上のE/H/S/U/R/Mから各敵の配置を読み込む。
	// 上下の地形を調べ、床側か天井側かを自動で決める。
	static bool LoadTileEnemies(
		int stageNumber,
		std::array<EnemySpawnData, StageData::MaxEnemySpawns>& enemySpawns,
		int& enemySpawnCount
	);

	// stageNumberに対応する Assets/Stages/stageN.txt から敵配置を読み込む。
	// [Enemies] の type x y HP surface [Affected|Fixed] [Left|Right] を読む。
	// 最後の項目がない旧形式はAffectedとして扱う。
	static bool LoadEnemies(
		int stageNumber,
		std::array<EnemySpawnData, StageData::MaxEnemySpawns>& enemySpawns,
		int& enemySpawnCount
	);

	// [MidBoss] TurtleGrenadier x y Floor|Ceiling Left|Right cameraGroupId [HP]
	static bool LoadMidBoss(
		int stageNumber,
		MidBossSpawnData& midBossSpawn,
		bool& hasMidBossSpawn
	);

	// [Boss] Tank x y Left|Right cameraGroupId [HP]
	static bool LoadBoss(
		int stageNumber,
		BossSpawnData& bossSpawn,
		bool& hasBossSpawn
	);

	// [Saws] の中心座標と反転方向を読み込む。旧形式の座標のみも有効。
	static bool LoadSaws(int stageNumber,
		std::vector<SawSpawnData>& sawSpawns,
		int& sawSpawnCount);

	// Mode、left、top、right、bottomと、省略可能なグループIDを読み込む。
	static bool LoadCameraAreas(
		int stageNumber,
		std::array<CameraArea, StageData::MaxCameraAreas>& cameraAreas,
		int& cameraAreaCount
	);

private:
	// ファイル内の文字を、ゲームで使えるタイル文字へ変換する。
	static char NormalizeTile(char tile);
};
