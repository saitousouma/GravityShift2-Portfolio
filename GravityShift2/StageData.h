#pragma once

#include <array>
#include <string>
#include <vector>

#include "CameraArea.h"
#include "BossSpawnData.h"
#include "EnemySpawnData.h"
#include "FallHole.h"
#include "GravityBlockSpawnData.h"
#include "MidBossSpawnData.h"
#include "SawSpawnData.h"
#include "StageClearCondition.h"
#include "Vector2.h"

// 画像の開始コマと再生コマ数。sheetIndexが-1なら従来のタイル描画を使う。
struct VisualTile
{
	short sheetIndex = -1;
	unsigned char column = 0;
	unsigned char row = 0;
	unsigned char frameCount = 1;
	bool flipX = false;
	bool flipY = false;
};

struct StageData
{
	static constexpr int MaxTileSheets = 64;
	static constexpr int MaxFallHoles = 32;
	static constexpr int MaxGravityBlockSpawns = 64;
	static constexpr int MaxEnemySpawns = 40;
	static constexpr int MaxCheckpoints = 16;
	static constexpr int MaxCameraAreas = 64;
	static constexpr int TileSize = 40;
	static constexpr int TileColumns = 256;
	static constexpr int DefaultTileRows = 18;
	static constexpr int TileRows = 96;
	static constexpr int StagePixelWidth = TileColumns * TileSize;
	static constexpr int MaxStagePixelHeight = TileRows * TileSize;
	// 穴の境界からこの距離だけ完全に抜けてから、反対側へ移動する。
	static constexpr int FallLoopTravelMargin = TileSize;
	using TileRow = std::array<char, TileColumns>;
	using VisualTileRow = std::array<VisualTile, TileColumns>;

	// ステージファイルで使うタイル記号。
	// ここに名前を付けておくと、記号の意味をコード全体で統一しやすい。
	static constexpr char EmptyTile = '.';
	static constexpr char BlockTile = '#';
	static constexpr char FloorTile = '=';
	static constexpr char WallTile = '|';
	static constexpr char InvisibleBlockTile = 'X';
	static constexpr char PlayerStartTile = 'P';
	static constexpr char GoalTile = 'G';
	static constexpr char GravityBlockTile = 'B';
	static constexpr char NormalEnemyTile = 'E';
	static constexpr char HeavyEnemyTile = 'H';
	static constexpr char ShooterEnemyTile = 'S';
	static constexpr char UpShooterEnemyTile = 'U';
	static constexpr char ChargerEnemyTile = 'R';
	static constexpr char MortarEnemyTile = 'M';
	static constexpr char CheckpointTile = 'K';

	int stageNumber = 1;
	// tiles配列のうち、実際のステージとして使っている行数。
	// 配列には余裕を持たせ、短いステージでは不要な行を処理しない。
	int tileRowCount = DefaultTileRows;
	ClearConditionType clearConditionType =
		ClearConditionType::ReachGoal;

	// 現在のステージの実際の高さをピクセル単位で返す。
	int GetPixelHeight() const
	{
		return tileRowCount * TileSize;
	}

	// 同じグループに属する全カメラ矩形を囲む範囲を返す。
	CameraArea GetCameraAreaGroupBounds(int areaIndex) const
	{
		if (areaIndex < 0 || areaIndex >= cameraAreaCount)
		{
			return {};
		}

		CameraArea bounds = cameraAreas[areaIndex];
		if (bounds.groupId < 0)
		{
			return bounds;
		}

		for (int i = 0; i < cameraAreaCount; ++i)
		{
			const CameraArea& part = cameraAreas[i];
			if (part.groupId != bounds.groupId)
			{
				continue;
			}
			if (part.left < bounds.left) bounds.left = part.left;
			if (part.top < bounds.top) bounds.top = part.top;
			if (part.right > bounds.right) bounds.right = part.right;
			if (part.bottom > bounds.bottom) bounds.bottom = part.bottom;
		}
		return bounds;
	}

	// 同じX位置で上下に並ぶ穴を1組の通路として扱う。
	// 下向きなら下側の床穴から上側の天井穴へ、上向きなら逆へ移動する。
	bool TryGetPairedFallHoleExitY(
		bool isMovingDown,
		float supportLeft,
		float supportRight,
		float objectTop,
		float objectBottom,
		float objectHeight,
		float& exitY) const
	{
		for (int sourceIndex = 0; sourceIndex < fallHoleCount; ++sourceIndex)
		{
			const FallHole& source = fallHoles[sourceIndex];
			if (source.IsLegacyBoundaryHole() ||
				!source.ContainsHorizontal(supportLeft, supportRight))
			{
				continue;
			}

			const bool passedSource = isMovingDown
				? objectTop >= static_cast<float>(
					source.bottom + FallLoopTravelMargin)
				: objectBottom <= static_cast<float>(
					source.top - FallLoopTravelMargin);
			if (!passedSource)
			{
				continue;
			}

			int targetIndex = -1;
			for (int candidateIndex = 0;
				candidateIndex < fallHoleCount; ++candidateIndex)
			{
				if (candidateIndex == sourceIndex)
				{
					continue;
				}

				const FallHole& candidate = fallHoles[candidateIndex];
				if (candidate.IsLegacyBoundaryHole() ||
					!candidate.ContainsHorizontal(supportLeft, supportRight))
				{
					continue;
				}

				const bool isOnExitSide = isMovingDown
					? candidate.bottom <= source.top
					: candidate.top >= source.bottom;
				if (!isOnExitSide)
				{
					continue;
				}

				if (targetIndex < 0 ||
					(isMovingDown && candidate.bottom >
						fallHoles[targetIndex].bottom) ||
					(!isMovingDown && candidate.top <
						fallHoles[targetIndex].top))
				{
					targetIndex = candidateIndex;
				}
			}

			if (targetIndex >= 0)
			{
				const FallHole& target = fallHoles[targetIndex];
				exitY = isMovingDown
					? static_cast<float>(target.bottom)
					: static_cast<float>(target.top) - objectHeight;
				return true;
			}
		}

		return false;
	}

	// 地形として扱うタイルかを判定する。
	// タイル種類が増えても、当たり判定側はこの関数を見ればよい。
	static bool IsSolidTile(char tile)
	{
		return tile == BlockTile ||
			tile == FloorTile ||
			tile == WallTile ||
			tile == InvisibleBlockTile;
	}

	// タイルマップ。
	// '.' は空間、'#' は汎用地形、'=' は横向き足場、'|' は縦向き壁として扱う。
	// P/G/B/E/H/S/U/R/Mは読み込み時に配置だけを拾い、タイルとしては空間に変換する。
	// 大きなタイル配列はヒープに置き、横長ステージでもスタックを圧迫しないようにする。
	std::vector<TileRow> tiles = std::vector<TileRow>(TileRows);

	// 地形の当たり判定と画像を分離し、背景画像は当たり判定を持たせない。
	std::array<std::string, MaxTileSheets> tileSheetNames{};
	int tileSheetCount = 0;
	std::vector<VisualTileRow> solidVisuals =
		std::vector<VisualTileRow>(TileRows);
	std::vector<VisualTileRow> backgroundVisuals =
		std::vector<VisualTileRow>(TileRows);

	// 落下穴の一覧。
	// fallHoleCount個分だけ有効な穴として扱う。
	std::array<FallHole, MaxFallHoles> fallHoles{};
	int fallHoleCount = 0;

	// 重力ブロックの初期配置一覧。
	// gravityBlockSpawnCount個分だけ有効な配置として扱う。
	std::array<GravityBlockSpawnData, MaxGravityBlockSpawns> gravityBlockSpawns{};
	int gravityBlockSpawnCount = 0;

	// 敵の初期配置一覧。
	// enemySpawnCount個分だけ有効な配置として扱う。
	std::array<EnemySpawnData, MaxEnemySpawns> enemySpawns{};
	int enemySpawnCount = 0;

	// 1ステージにつき中ボスは1体まで配置できる。
	MidBossSpawnData midBossSpawn{};
	bool hasMidBossSpawn = false;

	// 1ステージにつき戦車ボスは1体まで配置できる。
	BossSpawnData bossSpawn{};
	bool hasBossSpawn = false;

	// 回転ノコギリの中心座標と反転方向。
	std::vector<SawSpawnData> sawSpawns{};
	int sawSpawnCount = 0;

	// ゴールの配置。
	// hasGoalPositionがtrueなら、ステージファイルから読んだ位置を使う。
	Vector2 goalPosition{};
	bool hasGoalPosition = false;

	// プレイヤーの開始位置。
	// hasPlayerStartPositionがtrueなら、ステージファイルから読んだ位置を使う。
	Vector2 playerStartPosition{};
	bool hasPlayerStartPosition = false;

	// チェックポイントのタイル左上座標を復活位置として使用する。
	std::array<Vector2, MaxCheckpoints> checkpoints{};
	int checkpointCount = 0;

	// カメラ制御エリアの一覧。
	// cameraAreaCount個分だけ有効なエリアとして扱う。
	std::array<CameraArea, MaxCameraAreas> cameraAreas{};
	int cameraAreaCount = 0;
};
