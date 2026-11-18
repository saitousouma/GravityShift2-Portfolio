#pragma once

#include "StageData.h"
#include <string>
#include <vector>

// ステージの見た目を描画するクラス。
// Applicationから描画処理を分けることで、ステージ表現を変更しやすくする。
class StageRenderer
{
public:
	// ステージ描画で使う画像を読み込む。
	bool Load();
	int GetSheetCount() const;
	const std::string& GetSheetName(int index) const;
	int GetSheetColumns(int index) const;
	int GetSheetRows(int index) const;
	bool IsAnimatedSheet(int index) const;
	void UpdateAnimation();
	void ResetAnimation();
	void DrawSheet(int index, int left, int top, int right, int bottom) const;
	bool DrawSheetCell(int index, int column, int row, int frameCount,
		int left, int top, int right, int bottom,
		bool flipX = false, bool flipY = false) const;
	bool DrawVisualTile(const StageData& stageData, const VisualTile& tile,
		int left, int top, int right, int bottom) const;

	void Draw(const StageData& currentStage) const;

private:
	void DrawTileMap(const StageData& currentStage) const;
	void DrawTile(
		char tile,
		int left,
		int top,
		int right,
		int bottom
	) const;

	void DrawCeilingAndFloor(const StageData& currentStage) const;
	void DrawFallHoles(const StageData& currentStage) const;

	int tileSetHandle_ = -1;
	struct Sheet
	{
		std::string name;
		int handle = -1;
		int columns = 0;
		int rows = 0;
		bool isAnimated = false;
	};
	std::vector<Sheet> sheets_;
	unsigned int animationTick_ = 0;
};
