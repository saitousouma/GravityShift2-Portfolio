#pragma once

#include "EditorStageData.h"
#include "EditorHistory.h"
#include "StageValidator.h"
#include <array>
#include <string>

class InputManager;
class StageRenderer;

// ステージを画面上で編集するための管理クラス。
// 現段階では、ステージ確認と編集用カメラの移動を担当する。
class StageEditor
{
public:
	// 編集対象のステージに合わせてカメラを初期化する。
	void Initialize(const StageData& stageData, const StageRenderer* renderer);

	// キー入力から編集用カメラを更新する。
	void Update(const InputManager& inputManager);

	// ステージの簡易表示、グリッド、操作案内を描画する。
	void Draw() const;
	bool IsImageBrowserOpen() const { return isImageBrowserOpen_; }

	// 描画途中の通常タイルを履歴に確定し、編集中のデータを渡す。
	const StageData& PrepareForPreview();
	// マウス位置を一時的な開始地点にしたプレビュー用データを作る。
	bool TryPrepareCursorPreview(float playerWidth, float playerHeight,
		StageData& previewStage);
	void ResumeAfterPreview();

private:
	enum class SelectedObjectType { None, Enemy, GravityBlock, Saw, MidBoss, Boss };
	struct RecentVisualTile
	{
		int sheetIndex = -1;
		int sourceColumn = 0;
		int sourceRow = 0;
		int frameCount = 1;
		char role = StageData::FloorTile;
		bool flipX = false;
		bool flipY = false;
	};
	static constexpr int MaxRecentVisualTiles = 10;

	// カメラがステージ範囲外へ出ないように位置を補正する。
	void ClampCamera();
	void FinishStroke();
	void RefreshValidation();
	void FocusSelectedIssue();
	void UpdateImageBrowser(const InputManager& inputManager);
	void DrawImageBrowser() const;
	void SelectImageSheet(int index);
	void RememberCurrentVisualTile();
	void SelectRecentVisualTile(int index);

	// 右側の操作パネルに1種類分の選択ボタンを描画する。
	void DrawPaletteButton(
		int left,
		int top,
		const wchar_t* text,
		char tile
	) const;

	// プレイヤー開始地点やゴールを編集画面上へ描画する。
	void DrawObjectMarker(
		const Vector2& position,
		const wchar_t* text,
		int color
	) const;

	int cameraX_ = 0;
	int cameraY_ = 0;
	int worldWidth_ = 0;
	int worldHeight_ = 0;
	EditorStageData editorStageData_;
	EditorHistory history_;
	const StageRenderer* renderer_ = nullptr;
	bool isImageBrowserOpen_ = false;
	int selectedSheet_ = 0;
	int selectedSourceColumn_ = 0;
	int selectedSourceRow_ = 0;
	int selectedFrameCount_ = 1;
	bool hasSelectedSource_ = false;
	char selectedVisualRole_ = StageData::FloorTile;
	bool selectedVisualFlipX_ = false;
	bool selectedVisualFlipY_ = false;
	std::array<RecentVisualTile, MaxRecentVisualTiles> recentVisualTiles_{};
	int recentVisualTileCount_ = 0;
	std::string rectangleSheetName_;
	int rectangleSourceColumn_ = 0;
	int rectangleSourceRow_ = 0;
	int rectangleFrameCount_ = 1;
	char rectangleVisualRole_ = StageData::FloorTile;
	bool rectangleVisualFlipX_ = false;
	bool rectangleVisualFlipY_ = false;
	bool isStrokeActive_ = false;
	bool isStrokeDirty_ = false;
	bool isDraggingRectangle_ = false;
	bool isRectangleErase_ = false;
	int rectangleStartColumn_ = 0;
	int rectangleStartRow_ = 0;
	int rectangleEndColumn_ = 0;
	int rectangleEndRow_ = 0;
	char rectangleTile_ = StageData::FloorTile;
	char selectedTile_ = StageData::FloorTile;
	int previousMouseInput_ = 0;
	int selectedObjectColumn_ = 0;
	int selectedObjectRow_ = 0;
	SelectedObjectType selectedObjectType_ = SelectedObjectType::None;
	bool isDraggingObject_ = false;
	int fallHoleStartColumn_ = 0;
	int fallHoleEndColumn_ = 0;
	int fallHoleSurfaceRow_ = 0;
	bool isDraggingFallHole_ = false;
	int selectedCameraAreaIndex_ = -1;
	bool isDraggingCameraArea_ = false;
	bool isReplacingCameraArea_ = false;
	bool isAddingCameraAreaPart_ = false;
	int cameraAreaStartColumn_ = 0;
	int cameraAreaStartRow_ = 0;
	int cameraAreaEndColumn_ = 0;
	int cameraAreaEndRow_ = 0;
	int saveMessageTimer_ = 0;
	int previewMessageTimer_ = 0;
	int moveMessageTimer_ = 0;
	bool lastSaveSucceeded_ = false;
	bool showValidation_ = false;
	std::vector<StageValidationIssue> validationIssues_;
	int selectedIssueIndex_ = 0;

	// マウスが指しているタイルの列番号と行番号。
	int hoveredColumn_ = 0;
	int hoveredRow_ = 0;
	bool isHoveringStage_ = false;

	static constexpr int CameraMoveSpeed = 8;
	static constexpr int FastCameraMoveSpeed = 20;
	static constexpr char SelectTool = '\0';
	static constexpr char FallHoleTool = '!';
	static constexpr char CameraAreaTool = 'C';
	static constexpr char VisualBrushTool = '@';
	static constexpr char SawTool = '~';
	static constexpr char MidBossTool = '%';
	static constexpr char BossTool = '&';
	static constexpr char MovingGravityBlockTool = '^';
};
