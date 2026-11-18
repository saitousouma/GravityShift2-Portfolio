#pragma once

#include <initializer_list>

#include "StageData.h"

// 現在のプレイ中のステージ情報を管理するクラス。
class StageManager
{
public:
	// 最初のステージ情報を設定。
	void Initialize();

	void GoToNextStage();

	// 指定した番号のステージを開始する。
	void StartStage(int stageNumber);

	// 保存前の編集データをコピーしてテストプレイに使う。
	void StartPreview(const StageData& stageData);

	// 現在のステージ情報を取得。
	const StageData& GetCurrentStage() const;

	// 全ステージをクリアしたかどうかを判定する。
	bool IsGameCleared() const;

private:
	void SetupStage(int stageNumber);
	void SetupClearCondition();
	void SetupTileMap();
	void SetupFallHoles();
	void SetupGravityBlocks();
	void SetupTileObjectPositions();
	void SetupGoalPosition();
	void SetupPlayerStartPosition();
	void SetupCheckpoints();
	void SetupEnemies();
	void SetupMidBoss();
	void SetupBoss();
	void SetupCameraAreas();
	void ClearTileMap();
	void ClearFallHoles();
	void ClearGravityBlocks();
	void ClearEnemies();
	void ClearCameraAreas();
	void SetTileMap(std::initializer_list<const char*> rows);
	void AddFallHole(int left, int right);
	void AddGravityBlock(
		float x,
		float y,
		GravityBlockSpawnSurface surface = GravityBlockSpawnSurface::Floor
	);
	void AddCameraArea(
		CameraAreaMode mode,
		int left,
		int top,
		int right,
		int bottom
	);

	StageData currentStage_{};

	static constexpr int LastStageNumber = 5;

	bool isGameCleared_ = false;
};
