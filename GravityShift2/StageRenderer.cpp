#include "StageRenderer.h"

#include <algorithm>
#include <Windows.h>

#include "GameConfig.h"
#include "DxLib.h"

namespace
{
	constexpr int SourceTileSize = 32;
	constexpr unsigned int AnimationFrameTicks = 6;
	const std::string EmptySheetName;
}

// ステージ描画で使うタイルセット画像を読み込む。
// 読み込みに失敗しても、仮の矩形描画へ戻せるようにtrueを返す。
bool StageRenderer::Load()
{
	// 実行フォルダがプロジェクト直下でもx64/Debugでも探せるようにする。
	const wchar_t* searchRoots[][2] = {
		{ L"Assets/Images/TileMaps", L"../../Assets/Images/TileMaps" },
		{ L"Assets/Images/AnimatedTiles", L"../../Assets/Images/AnimatedTiles" }
	};
	for (int category = 0; category < 2; ++category)
	{
	for (const wchar_t* root : searchRoots[category])
	{
		const std::wstring pattern = std::wstring(root) + L"/*.png";
		WIN32_FIND_DATAW findData{};
		HANDLE search = FindFirstFileW(pattern.c_str(), &findData);
		if (search == INVALID_HANDLE_VALUE)
		{
			continue;
		}
		std::vector<std::wstring> names;
		do
		{
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				names.push_back(findData.cFileName);
			}
		} while (FindNextFileW(search, &findData));
		FindClose(search);
		std::sort(names.begin(), names.end());

		for (const std::wstring& wideName : names)
		{
			const std::wstring path = std::wstring(root) + L"/" + wideName;
			const int handle = LoadGraph(path.c_str());
			int width = 0;
			int height = 0;
			if (handle == -1 || GetGraphSize(handle, &width, &height) != 0 ||
				width < SourceTileSize || height < SourceTileSize ||
				width % SourceTileSize != 0 || height % SourceTileSize != 0)
			{
				if (handle != -1) DeleteGraph(handle);
				continue;
			}
			// ステージファイルの画像名はUTF-8で保存する。
			const int byteCount = WideCharToMultiByte(CP_UTF8, 0,
				wideName.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (byteCount <= 1)
			{
				DeleteGraph(handle);
				continue;
			}
			std::string name(byteCount, '\0');
			WideCharToMultiByte(CP_UTF8, 0, wideName.c_str(), -1,
				&name[0], byteCount, nullptr, nullptr);
			name.pop_back();
			sheets_.push_back({ name, handle,
				width / SourceTileSize, height / SourceTileSize,
				category == 1 });
			if (name == "1_Industrial_Tileset_1.png")
			{
				tileSetHandle_ = handle;
			}
		}
		break;
	}
	}

	return true;
}

int StageRenderer::GetSheetCount() const
{
	return static_cast<int>(sheets_.size());
}

const std::string& StageRenderer::GetSheetName(int index) const
{
	return index >= 0 && index < GetSheetCount()
		? sheets_[index].name : EmptySheetName;
}

int StageRenderer::GetSheetColumns(int index) const
{
	return index >= 0 && index < GetSheetCount() ? sheets_[index].columns : 0;
}

int StageRenderer::GetSheetRows(int index) const
{
	return index >= 0 && index < GetSheetCount() ? sheets_[index].rows : 0;
}

bool StageRenderer::IsAnimatedSheet(int index) const
{
	return index >= 0 && index < GetSheetCount() && sheets_[index].isAnimated;
}

void StageRenderer::UpdateAnimation()
{
	++animationTick_;
}

void StageRenderer::ResetAnimation()
{
	animationTick_ = 0;
}

void StageRenderer::DrawSheet(int index, int left, int top,
	int right, int bottom) const
{
	if (index >= 0 && index < GetSheetCount())
	{
		DrawExtendGraph(left, top, right, bottom, sheets_[index].handle, TRUE);
	}
}

bool StageRenderer::DrawSheetCell(int index, int column, int row, int frameCount,
	int left, int top, int right, int bottom, bool flipX, bool flipY) const
{
	if (index < 0 || index >= static_cast<int>(sheets_.size()) ||
		column < 0 || row < 0 || frameCount < 1)
	{
		return false;
	}

	const Sheet& sheet = sheets_[index];
	const int firstFrame = row * sheet.columns + column;
	if (column >= sheet.columns || row >= sheet.rows ||
		firstFrame + frameCount > sheet.columns * sheet.rows)
	{
		return false;
	}

	const int frame = firstFrame +
		(animationTick_ / AnimationFrameTicks) % frameCount;
	const int sourceLeftX = flipX ? right : left;
	const int sourceRightX = flipX ? left : right;
	const int sourceTopY = flipY ? bottom : top;
	const int sourceBottomY = flipY ? top : bottom;
	DrawRectModiGraph(
		sourceLeftX, sourceTopY,
		sourceRightX, sourceTopY,
		sourceRightX, sourceBottomY,
		sourceLeftX, sourceBottomY,
		(frame % sheet.columns) * SourceTileSize,
		(frame / sheet.columns) * SourceTileSize,
		SourceTileSize, SourceTileSize, sheet.handle, TRUE);
	return true;
}

bool StageRenderer::DrawVisualTile(const StageData& stageData,
	const VisualTile& tile, int left, int top, int right, int bottom) const
{
	if (tile.sheetIndex < 0 || tile.sheetIndex >= stageData.tileSheetCount)
	{
		return false;
	}
	const std::string& name = stageData.tileSheetNames[tile.sheetIndex];
	for (const Sheet& sheet : sheets_)
	{
		if (sheet.name != name)
		{
			continue;
		}
		const int firstFrame = static_cast<int>(tile.row) * sheet.columns + tile.column;
		if (tile.frameCount == 0 || tile.column >= sheet.columns ||
			tile.row >= sheet.rows ||
			firstFrame + tile.frameCount > sheet.columns * sheet.rows)
		{
			return false;
		}
		const int frame = firstFrame +
			(animationTick_ / AnimationFrameTicks) % tile.frameCount;
		const int sourceLeftX = tile.flipX ? right : left;
		const int sourceRightX = tile.flipX ? left : right;
		const int sourceTopY = tile.flipY ? bottom : top;
		const int sourceBottomY = tile.flipY ? top : bottom;
		DrawRectModiGraph(
			sourceLeftX, sourceTopY,
			sourceRightX, sourceTopY,
			sourceRightX, sourceBottomY,
			sourceLeftX, sourceBottomY,
			(frame % sheet.columns) * SourceTileSize,
			(frame / sheet.columns) * SourceTileSize,
			SourceTileSize, SourceTileSize, sheet.handle, TRUE);
		return true;
	}
	return false;
}

// ステージ全体の描画入口。
// タイルマップ、床・天井の穴を描画する。
void StageRenderer::Draw(const StageData& currentStage) const
{
	DrawTileMap(currentStage);
	DrawFallHoles(currentStage);
}

// StageDataのタイル配列をもとに、壁や床をタイルとして描画する。
void StageRenderer::DrawTileMap(const StageData& currentStage) const
{
	// 背景は地形の後ろへ描く。ここには当たり判定を作らない。
	for (int y = 0; y < currentStage.tileRowCount; ++y)
	{
		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			const int left = x * StageData::TileSize;
			const int top = y * StageData::TileSize;
			DrawVisualTile(currentStage, currentStage.backgroundVisuals[y][x],
				left, top, left + StageData::TileSize, top + StageData::TileSize);
		}
	}

	for (int y = 0; y < currentStage.tileRowCount; ++y)
	{
		for (int x = 0; x < StageData::TileColumns; ++x)
		{
			const char tile = currentStage.tiles[y][x];

			if (!StageData::IsSolidTile(tile))
			{
				continue;
			}
			if (tile == StageData::InvisibleBlockTile)
			{
				continue;
			}

			const int left = x * StageData::TileSize;
			const int top = y * StageData::TileSize;
			const int right = left + StageData::TileSize;
			const int bottom = top + StageData::TileSize;

			if (!DrawVisualTile(currentStage, currentStage.solidVisuals[y][x],
				left, top, right, bottom))
			{
				DrawTile(tile, left, top, right, bottom);
			}
		}
	}
}

// タイルセット画像から1マス分を切り出して描画する。
// タイル文字に応じて切り出し位置を変え、床、壁、汎用ブロックを描き分ける。
void StageRenderer::DrawTile(
	char tile,
	int left,
	int top,
	int right,
	int bottom) const
{
	if (tileSetHandle_ != -1)
	{
		const int sourceSize = 32;
		int sourceColumn = 0;
		int sourceRow = 0;

		if (tile == StageData::FloorTile)
		{
			// 横向きの床や浮遊足場に使うタイル。
			sourceColumn = 1;
			sourceRow = 0;
		}
		else if (tile == StageData::WallTile)
		{
			// 左右の壁に使うタイル。
			sourceColumn = 5;
			sourceRow = 0;
		}
		else
		{
			// 角や詰め物に使う汎用タイル。
			sourceColumn = 0;
			sourceRow = 0;
		}

		DrawRectExtendGraph(
			left,
			top,
			right,
			bottom,
			sourceColumn * sourceSize,
			sourceRow * sourceSize,
			sourceSize,
			sourceSize,
			tileSetHandle_,
			TRUE
		);

		return;
	}

	const int tileBaseColor = GetColor(30, 120, 150);
	const int tileLightColor = GetColor(80, 220, 255);
	const int tileDarkColor = GetColor(15, 45, 70);
	int fillColor = tileBaseColor;

	if (tile == StageData::FloorTile)
	{
		fillColor = GetColor(35, 135, 170);
	}
	else if (tile == StageData::WallTile)
	{
		fillColor = GetColor(20, 90, 120);
	}

	// 画像が読み込めなかった場合は、今までの仮タイル表示に戻す。
	DrawBox(
		left,
		top,
		right,
		bottom,
		fillColor,
		TRUE
	);

	DrawBox(
		left,
		top,
		right,
		bottom,
		tileLightColor,
		FALSE
	);

	DrawBox(
		left + 5,
		top + 5,
		right - 5,
		bottom - 5,
		tileDarkColor,
		FALSE
	);
}

// 天井と床を描画する。
// 穴がある場所だけ描画しないことで、落下できる隙間を作る。
void StageRenderer::DrawCeilingAndFloor(
	const StageData& currentStage) const
{
	const int drawLeft = 0;
	const int drawRight = GameConfig::ScreenWidth;
	int segmentLeft = drawLeft;

	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];

		if (segmentLeft < fallHole.left)
		{
			DrawBox(
				segmentLeft,
				0,
				fallHole.left,
				GameConfig::CeilingY,
				GetColor(80, 80, 100),
				TRUE
			);

			DrawBox(
				segmentLeft,
				GameConfig::FloorY,
				fallHole.left,
				GameConfig::ScreenHeight,
				GetColor(80, 80, 100),
				TRUE
			);
		}

		segmentLeft = fallHole.right;
	}

	if (segmentLeft < drawRight)
	{
		DrawBox(
			segmentLeft,
			0,
			drawRight,
			GameConfig::CeilingY,
			GetColor(80, 80, 100),
			TRUE
		);

		DrawBox(
			segmentLeft,
			GameConfig::FloorY,
			drawRight,
			GameConfig::ScreenHeight,
			GetColor(80, 80, 100),
			TRUE
		);
	}
}

// 落下穴の見た目を描画する。
// 判定用の穴データと同じ範囲を使うので、見た目と当たり判定がずれにくい。
void StageRenderer::DrawFallHoles(
	const StageData& currentStage) const
{
	const int holeDarkColor = GetColor(15, 15, 25);
	const int holeEdgeColor = GetColor(255, 80, 80);
	const int holeInnerLineColor = GetColor(120, 40, 70);
	const int boundaryThickness = StageData::TileSize * 2;
	const int stageBottom = currentStage.GetPixelHeight();

	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& fallHole = currentStage.fallHoles[i];
		const int holeLeft = fallHole.left;
		const int holeRight = fallHole.right;
		const int holeCenterX = (holeLeft + holeRight) / 2;
		auto drawHoleRect = [&](int holeTop, int holeBottom)
		{
			DrawBox(holeLeft, holeTop, holeRight, holeBottom,
				holeDarkColor, TRUE);
			DrawBox(holeLeft - 4, holeTop, holeLeft, holeBottom,
				holeEdgeColor, TRUE);
			DrawBox(holeRight, holeTop, holeRight + 4, holeBottom,
				holeEdgeColor, TRUE);
			DrawLine(holeCenterX, holeTop, holeCenterX, holeBottom,
				holeInnerLineColor);
		};

		if (fallHole.IsLegacyBoundaryHole())
		{
			drawHoleRect(0, boundaryThickness);
			drawHoleRect(stageBottom - boundaryThickness, stageBottom);
		}
		else
		{
			drawHoleRect(fallHole.top, fallHole.bottom);
		}
	}
}
