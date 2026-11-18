#pragma once

#include "StageData.h"
#include <string>

// エディターで変更するステージデータを保持するクラス。
// ゲーム本編が使用中のStageDataを直接書き換えないための作業用コピーになる。
class EditorStageData
{
public:
	// 読み込まれているステージを編集用データへコピーする。
	void LoadFrom(const StageData& sourceStageData);

	// 指定したマスへ、地形または特殊オブジェクトを配置する。
	// 実際に内容が変わった場合だけtrueを返す。
	bool Place(int column, int row, char item);
	// 画像コマを配置する。背景は地形を変えず、他の役割は物理タイルも更新する。
	bool PlaceVisual(int column, int row, const std::string& sheetName,
		int sourceColumn, int sourceRow, char role, int frameCount = 1,
		bool flipX = false, bool flipY = false);
	bool FillVisualRectangle(int firstColumn, int firstRow,
		int lastColumn, int lastRow, const std::string& sheetName,
		int sourceColumn, int sourceRow, char role, int frameCount = 1,
		bool flipX = false, bool flipY = false);
	// 地形3種類だけを矩形内へ配置する。変更がなければfalseを返す。
	bool FillRectangle(int firstColumn, int firstRow,
		int lastColumn, int lastRow, char tile);
	// 矩形内を通常の右クリック消去と同じルールで空にする。
	bool EraseRectangle(int firstColumn, int firstRow,
		int lastColumn, int lastRow);

	// 指定したマスの地形と特殊オブジェクトを削除する。
	bool Erase(int column, int row);

	// 描画や保存で利用する編集用ステージデータを返す。
	const StageData& GetStageData() const;
	// ステージ下端へ空行を追加する。最大行数を超えた分は切り詰める。
	bool ExpandRows(int rowCount);

	// 指定したマスに敵がいるか調べる。
	bool HasEnemyAt(int column, int row) const;
	bool HasGravityBlockAt(int column, int row) const;
	bool HasSawAt(int column, int row) const;
	bool HasCheckpointAt(int column, int row) const;
	bool HasMidBossAt(int column, int row) const;
	bool HasBossAt(int column, int row) const;
	bool PlaceCheckpoint(int column, int row);
	bool PlaceMidBoss(int column, int row);
	bool MoveMidBoss(int fromColumn, int fromRow, int toColumn, int toRow);
	bool ToggleMidBossFacingAt(int column, int row);
	bool PlaceBoss(int column, int row);
	bool MoveBoss(int fromColumn, int fromRow, int toColumn, int toRow);
	bool ToggleBossFacingAt(int column, int row);
	bool PlaceSaw(int column, int row);
	bool PlaceMovingGravityBlock(int column, int row);
	bool MoveSaw(int fromColumn, int fromRow, int toColumn, int toRow);
	bool FlipSawAt(int column, int row, bool horizontal);
	const SawSpawnData* GetSawAt(int column, int row) const;
	bool MoveEnemy(int fromColumn, int fromRow, int toColumn, int toRow);
	bool MoveGravityBlock(int fromColumn, int fromRow, int toColumn, int toRow);
	GravityBlockMotion GetGravityBlockMotionAt(int column, int row) const;
	bool ToggleGravityBlockMotionAt(int column, int row);

	// 指定したマスにいる敵のHPを返す。敵がいなければ0を返す。
	int GetEnemyHitPointAt(int column, int row) const;

	// 指定したマスにいる敵のHPを増減する。
	bool ChangeEnemyHitPointAt(int column, int row, int amount);
	// 中ボスまたはボスのHPを取得・増減する。
	int GetBossHitPointAt(int column, int row) const;
	bool ChangeBossHitPointAt(int column, int row, int amount);
	// 敵が重力反転の影響を受ける設定か返し、Gキー用に切り替える。
	bool IsEnemyAffectedByGravityReversalAt(int column, int row) const;
	bool ToggleEnemyGravityResponseAt(int column, int row);
	bool IsEnemyFacingRightAt(int column, int row) const;
	bool ToggleEnemyFacingAt(int column, int row);

	// 開始列から終了列までを、床と天井をつなぐ穴として登録する。
	bool AddFallHole(int startColumn, int endColumn, int surfaceRow = -1);

	// 指定した列を含む穴を削除し、床と天井を閉じる。
	bool RemoveFallHoleAt(int column, int row = -1);

	// カメラ制御エリアは後に追加したものを優先して選択する。
	int FindCameraAreaAt(int column, int row) const;
	bool AddCameraArea(int firstColumn, int firstRow, int lastColumn, int lastRow);
	bool AddCameraAreaPart(int sourceIndex, int firstColumn, int firstRow,
		int lastColumn, int lastRow);
	bool ReplaceCameraArea(int index, int firstColumn, int firstRow,
		int lastColumn, int lastRow);
	bool CycleCameraAreaMode(int index);
	bool RemoveCameraArea(int index);

private:
	int GetNextCameraAreaGroupId() const;
	bool IsInsideStage(int column, int row) const;
	bool IsPositionInCell(
		const Vector2& position,
		int column,
		int row
	) const;
	bool IsCeilingPlacement(int column, int row) const;
	bool CanMoveObjectTo(int column, int row, float width, float height,
		int ignoredEnemyIndex, int ignoredBlockIndex,
		bool ignoreMidBoss = false, bool ignoreBoss = false,
		bool forceFloor = false) const;
	int FindGravityBlockAt(int column, int row) const;
	bool RemoveGravityBlockAt(int column, int row);
	bool PlaceGravityBlock(int column, int row,
		GravityBlockMotion motion = GravityBlockMotion::Gravity);
	int FindEnemyAt(int column, int row) const;
	bool RemoveEnemyAt(int column, int row);
	bool PlaceEnemy(int column, int row, char item);
	int FindSawAt(int column, int row) const;
	bool RemoveSawAt(int column, int row);
	int FindCheckpointAt(int column, int row) const;
	bool RemoveCheckpointAt(int column, int row);
	bool RemoveMidBossAt(int column, int row);
	bool RemoveBossAt(int column, int row);
	bool SetTile(int column, int row, char tile);

	StageData stageData_{};
};
