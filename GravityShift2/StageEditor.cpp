#include "StageEditor.h"

#include <algorithm>
#include <Windows.h>

#include "GameConfig.h"
#include "InputManager.h"
#include "StagePreviewSpawn.h"
#include "StageFileWriter.h"
#include "StageRenderer.h"
#include "UiFont.h"
#include "DxLib.h"

namespace
{
	constexpr int EditorToolbarHeight = 56;
	constexpr int EditorSidebarWidth = 320;
	constexpr int EditorViewportWidth =
		GameConfig::ScreenWidth - EditorSidebarWidth;
	constexpr int PaletteButtonHeight = 30;
	constexpr int PaletteButtonWidth = 136;
	constexpr int SidebarFirstColumn = EditorViewportWidth + 12;
	constexpr int SidebarSecondColumn = EditorViewportWidth + 160;
	constexpr int TerrainFirstRow = 82;
	constexpr int TerrainSecondRow = 114;
	constexpr int ObjectFirstRow = 168;
	constexpr int ObjectSecondRow = 200;
	constexpr int ObjectThirdRow = 232;
	constexpr int ObjectFourthRow = 264;
	constexpr int EnemyFirstRow = 320;
	constexpr int EnemySecondRow = 352;
	constexpr int EnemyThirdRow = 384;
	constexpr int EnemyFourthRow = 416;
	constexpr int ImageBrowserButtonTop = 456;
	constexpr int BrowserListTop = 188;
	constexpr int BrowserPageSize = 15;
	constexpr int BrowserPreviewLeft = 405;
	constexpr int BrowserPreviewTop = 230;
	constexpr int BrowserPreviewWidth = 700;
	constexpr int BrowserPreviewHeight = 400;
	constexpr int BrowserSelectedPreviewSize = 100;
	constexpr int BrowserSelectedPreviewLeft =
		GameConfig::ScreenWidth - BrowserSelectedPreviewSize - 24;
	constexpr int BrowserSelectedPreviewTop = BrowserPreviewTop;
	constexpr int BrowserRecentLeft = 535;
	constexpr int BrowserRecentTop = 112;
	constexpr int BrowserRecentSize = 40;
	constexpr int BrowserRecentGap = 6;

	bool IsInsidePaletteButton(int mouseX, int mouseY, int left, int top)
	{
		return mouseX >= left && mouseX < left + PaletteButtonWidth &&
			mouseY >= top && mouseY < top + PaletteButtonHeight;
	}

	const wchar_t* GetCameraAreaModeText(CameraAreaMode mode)
	{
		switch (mode)
		{
		case CameraAreaMode::Horizontal: return L"横追従";
		case CameraAreaMode::Vertical: return L"縦追従";
		case CameraAreaMode::Locked: return L"固定";
		default: return L"自由";
		}
	}

	// valueをminValue以上、maxValue以下に収める。
	int ClampInt(int value, int minValue, int maxValue)
	{
		if (value < minValue)
		{
			return minValue;
		}

		if (value > maxValue)
		{
			return maxValue;
		}

		return value;
	}

	std::wstring ToWide(const std::string& utf8)
	{
		const int count = MultiByteToWideChar(CP_UTF8, 0,
			utf8.c_str(), -1, nullptr, 0);
		if (count <= 1)
		{
			return {};
		}
		std::wstring wide(count, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1,
			&wide[0], count);
		wide.pop_back();
		return wide;
	}
}

// 編集対象の大きさを保存し、カメラを左上へ戻す。
void StageEditor::Initialize(const StageData& stageData,
	const StageRenderer* renderer)
{
	renderer_ = renderer;
	isImageBrowserOpen_ = false;
	selectedSheet_ = 0;
	selectedSourceColumn_ = 0;
	selectedSourceRow_ = 0;
	selectedFrameCount_ = 1;
	hasSelectedSource_ = false;
	selectedVisualRole_ = StageData::FloorTile;
	selectedVisualFlipX_ = false;
	selectedVisualFlipY_ = false;
	editorStageData_.LoadFrom(stageData);
	history_.Reset(stageData);
	cameraX_ = 0;
	cameraY_ = 0;
	worldWidth_ = StageData::StagePixelWidth;
	worldHeight_ = stageData.GetPixelHeight();
	isStrokeActive_ = false;
	isStrokeDirty_ = false;
	isDraggingRectangle_ = false;
	isRectangleErase_ = false;
	selectedTile_ = StageData::FloorTile;
	previousMouseInput_ = GetMouseInput();
	selectedObjectColumn_ = 0;
	selectedObjectRow_ = 0;
	selectedObjectType_ = SelectedObjectType::None;
	isDraggingObject_ = false;
	fallHoleStartColumn_ = 0;
	fallHoleEndColumn_ = 0;
	fallHoleSurfaceRow_ = 0;
	isDraggingFallHole_ = false;
	selectedCameraAreaIndex_ = -1;
	isDraggingCameraArea_ = false;
	isReplacingCameraArea_ = false;
	saveMessageTimer_ = 0;
	previewMessageTimer_ = 0;
	moveMessageTimer_ = 0;
	lastSaveSucceeded_ = false;
	showValidation_ = false;
	selectedIssueIndex_ = 0;
	RefreshValidation();
	hoveredColumn_ = 0;
	hoveredRow_ = 0;
	isHoveringStage_ = false;
	ClampCamera();
}

// 矢印キーまたはWASDキーで、編集用カメラを移動する。
void StageEditor::Update(const InputManager& inputManager)
{
	if (saveMessageTimer_ > 0)
	{
		--saveMessageTimer_;
	}
	if (previewMessageTimer_ > 0)
	{
		--previewMessageTimer_;
	}
	if (moveMessageTimer_ > 0)
	{
		--moveMessageTimer_;
	}

	const bool isControlPressed =
		inputManager.IsPressed(KEY_INPUT_LCONTROL) ||
		inputManager.IsPressed(KEY_INPUT_RCONTROL);

	if (inputManager.IsTriggered(KEY_INPUT_I))
	{
		FinishStroke();
		isDraggingRectangle_ = false;
		isImageBrowserOpen_ = !isImageBrowserOpen_;
		previousMouseInput_ = GetMouseInput();
		return;
	}
	if (isImageBrowserOpen_)
	{
		if (inputManager.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isImageBrowserOpen_ = false;
			previousMouseInput_ = GetMouseInput();
			return;
		}
		UpdateImageBrowser(inputManager);
		return;
	}

	// ショートカットは描画操作より先に処理し、同じフレームの誤配置を防ぐ。
	if (isControlPressed &&
		(inputManager.IsTriggered(KEY_INPUT_Z) ||
		inputManager.IsTriggered(KEY_INPUT_Y) ||
		inputManager.IsTriggered(KEY_INPUT_S)))
	{
		FinishStroke();
		isDraggingRectangle_ = false;
		isDraggingFallHole_ = false;
		isDraggingCameraArea_ = false;
		isDraggingObject_ = false;
		previousMouseInput_ = GetMouseInput();

		if (inputManager.IsTriggered(KEY_INPUT_Z))
		{
			history_.Undo(editorStageData_);
			selectedObjectType_ = SelectedObjectType::None;
			selectedCameraAreaIndex_ = -1;
		}
		else if (inputManager.IsTriggered(KEY_INPUT_Y))
		{
			history_.Redo(editorStageData_);
			selectedObjectType_ = SelectedObjectType::None;
			selectedCameraAreaIndex_ = -1;
		}
		else
		{
			RefreshValidation();
			showValidation_ = !validationIssues_.empty();
			lastSaveSucceeded_ = StageFileWriter::Save(
				editorStageData_.GetStageData());
			saveMessageTimer_ = 120;
			if (lastSaveSucceeded_)
			{
				history_.MarkSaved();
			}
		}
		RefreshValidation();
		worldHeight_ = editorStageData_.GetStageData().GetPixelHeight();
		ClampCamera();

		return;
	}

	if (inputManager.IsTriggered(KEY_INPUT_V))
	{
		FinishStroke();
		RefreshValidation();
		showValidation_ = !showValidation_;
	}
	if (showValidation_ && !validationIssues_.empty() &&
		inputManager.IsTriggered(KEY_INPUT_TAB))
	{
		selectedIssueIndex_ =
			(selectedIssueIndex_ + 1) % static_cast<int>(validationIssues_.size());
		FocusSelectedIssue();
	}
	if (inputManager.IsTriggered(KEY_INPUT_PGDN))
	{
		FinishStroke();
		if (editorStageData_.ExpandRows(8))
		{
			history_.Commit(editorStageData_.GetStageData());
			worldHeight_ = editorStageData_.GetStageData().GetPixelHeight();
			RefreshValidation();
			ClampCamera();
		}
	}

	// 数字キーから配置するタイルを切り替える。
	if (inputManager.IsTriggered(KEY_INPUT_1))
	{
		selectedTile_ = StageData::FloorTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_2))
	{
		selectedTile_ = StageData::WallTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_3))
	{
		selectedTile_ = StageData::BlockTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_4))
	{
		selectedTile_ = StageData::PlayerStartTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_5))
	{
		selectedTile_ = StageData::GoalTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_6))
	{
		selectedTile_ = StageData::GravityBlockTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_7))
	{
		selectedTile_ = StageData::NormalEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_8))
	{
		selectedTile_ = StageData::HeavyEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_9))
	{
		selectedTile_ = StageData::ShooterEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_0))
	{
		selectedTile_ = SelectTool;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_F))
	{
		selectedTile_ = FallHoleTool;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_C))
	{
		selectedTile_ = CameraAreaTool;
		selectedObjectType_ = SelectedObjectType::None;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_Q))
	{
		selectedTile_ = SawTool;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_H))
	{
		selectedTile_ = MovingGravityBlockTool;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_N))
	{
		selectedTile_ = StageData::InvisibleBlockTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_K))
	{
		selectedTile_ = StageData::CheckpointTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_J))
	{
		selectedTile_ = StageData::ChargerEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_L))
	{
		selectedTile_ = StageData::MortarEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_U))
	{
		selectedTile_ = StageData::UpShooterEnemyTile;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_O))
	{
		selectedTile_ = MidBossTool;
	}
	else if (inputManager.IsTriggered(KEY_INPUT_B))
	{
		selectedTile_ = BossTool;
	}

	if (selectedTile_ == VisualBrushTool && hasSelectedSource_ &&
		!isControlPressed)
	{
		if (inputManager.IsTriggered(KEY_INPUT_X))
		{
			selectedVisualFlipX_ = !selectedVisualFlipX_;
		}
		if (inputManager.IsTriggered(KEY_INPUT_Y))
		{
			selectedVisualFlipY_ = !selectedVisualFlipY_;
		}
	}

	const int moveSpeed = inputManager.IsPressed(KEY_INPUT_LSHIFT)
		? FastCameraMoveSpeed
		: CameraMoveSpeed;

	if (inputManager.IsPressed(KEY_INPUT_LEFT) ||
		inputManager.IsPressed(KEY_INPUT_A))
	{
		cameraX_ -= moveSpeed;
	}

	if (inputManager.IsPressed(KEY_INPUT_RIGHT) ||
		inputManager.IsPressed(KEY_INPUT_D))
	{
		cameraX_ += moveSpeed;
	}

	if (inputManager.IsPressed(KEY_INPUT_UP) ||
		inputManager.IsPressed(KEY_INPUT_W))
	{
		cameraY_ -= moveSpeed;
	}

	if (inputManager.IsPressed(KEY_INPUT_DOWN) ||
		(!isControlPressed && inputManager.IsPressed(KEY_INPUT_S)))
	{
		cameraY_ += moveSpeed;
	}

	ClampCamera();

	// マウスの画面座標へカメラ位置を加え、ステージ上の座標に変換する。
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);
	const int mouseInput = GetMouseInput();
	const bool isLeftTriggered =
		(mouseInput & MOUSE_INPUT_LEFT) != 0 &&
		(previousMouseInput_ & MOUSE_INPUT_LEFT) == 0;
	const bool isLeftReleased =
		(mouseInput & MOUSE_INPUT_LEFT) == 0 &&
		(previousMouseInput_ & MOUSE_INPUT_LEFT) != 0;
	const bool isRightTriggered =
		(mouseInput & MOUSE_INPUT_RIGHT) != 0 &&
		(previousMouseInput_ & MOUSE_INPUT_RIGHT) == 0;
	const bool isRightReleased =
		(mouseInput & MOUSE_INPUT_RIGHT) == 0 &&
		(previousMouseInput_ & MOUSE_INPUT_RIGHT) != 0;

	// 右側パネルのボタンも、表示されているキーと同じ選択にする。
	if (isLeftTriggered)
	{
		if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, TerrainFirstRow))
		{
			selectedTile_ = StageData::FloorTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, TerrainFirstRow))
		{
			selectedTile_ = StageData::WallTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, TerrainSecondRow))
		{
			selectedTile_ = StageData::BlockTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, TerrainSecondRow))
		{
			selectedTile_ = StageData::InvisibleBlockTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, ObjectFirstRow))
		{
			selectedTile_ = StageData::PlayerStartTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, ObjectFirstRow))
		{
			selectedTile_ = StageData::GoalTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, ObjectSecondRow))
		{
			selectedTile_ = StageData::GravityBlockTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, ObjectSecondRow))
		{
			selectedTile_ = SawTool;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, ObjectThirdRow))
		{
			selectedTile_ = FallHoleTool;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, ObjectThirdRow))
		{
			selectedTile_ = CameraAreaTool;
			selectedObjectType_ = SelectedObjectType::None;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, ObjectFourthRow))
		{
			selectedTile_ = StageData::CheckpointTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, ObjectFourthRow))
		{
			selectedTile_ = SelectTool;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, EnemyFirstRow))
		{
			selectedTile_ = StageData::NormalEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, EnemyFirstRow))
		{
			selectedTile_ = StageData::HeavyEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, EnemySecondRow))
		{
			selectedTile_ = StageData::ShooterEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, EnemySecondRow))
		{
			selectedTile_ = StageData::UpShooterEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, EnemyThirdRow))
		{
			selectedTile_ = StageData::ChargerEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, EnemyThirdRow))
		{
			selectedTile_ = StageData::MortarEnemyTile;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, EnemyFourthRow))
		{
			selectedTile_ = MidBossTool;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, EnemyFourthRow))
		{
			selectedTile_ = BossTool;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarFirstColumn, ImageBrowserButtonTop))
		{
			FinishStroke();
			isDraggingRectangle_ = false;
			isImageBrowserOpen_ = true;
			previousMouseInput_ = mouseInput;
			return;
		}
		else if (IsInsidePaletteButton(mouseX, mouseY,
			SidebarSecondColumn, ImageBrowserButtonTop))
		{
			selectedTile_ = MovingGravityBlockTool;
		}
	}

	const int worldMouseX = mouseX + cameraX_;
	const int worldMouseY =
		mouseY - EditorToolbarHeight + cameraY_;

	isHoveringStage_ =
		mouseX >= 0 && mouseX < EditorViewportWidth &&
		mouseY >= EditorToolbarHeight &&
		mouseY < GameConfig::ScreenHeight &&
		worldMouseX >= 0 && worldMouseX < worldWidth_ &&
		worldMouseY >= 0 && worldMouseY < worldHeight_;

	if (isHoveringStage_)
	{
		hoveredColumn_ = worldMouseX / StageData::TileSize;
		hoveredRow_ = worldMouseY / StageData::TileSize;
		const bool isTerrainTool =
			selectedTile_ == StageData::FloorTile ||
			selectedTile_ == StageData::WallTile ||
			selectedTile_ == StageData::BlockTile ||
			selectedTile_ == StageData::InvisibleBlockTile ||
			selectedTile_ == VisualBrushTool;
		const bool isShiftPressed =
			inputManager.IsPressed(KEY_INPUT_LSHIFT) ||
			inputManager.IsPressed(KEY_INPUT_RSHIFT);
		const bool startRectangleFill = isLeftTriggered && isTerrainTool &&
			isShiftPressed && (mouseInput & MOUSE_INPUT_RIGHT) == 0;
		const bool startRectangleErase = isRightTriggered && isShiftPressed &&
			selectedTile_ != FallHoleTool && selectedTile_ != CameraAreaTool &&
			(mouseInput & MOUSE_INPUT_LEFT) == 0;
		if (!isDraggingRectangle_ &&
			(startRectangleFill || startRectangleErase))
		{
			FinishStroke();
			isDraggingRectangle_ = true;
			isRectangleErase_ = startRectangleErase;
			rectangleStartColumn_ = rectangleEndColumn_ = hoveredColumn_;
			rectangleStartRow_ = rectangleEndRow_ = hoveredRow_;
			rectangleTile_ = selectedTile_;
			if (selectedTile_ == VisualBrushTool && renderer_)
			{
				rectangleSheetName_ = renderer_->GetSheetName(selectedSheet_);
				rectangleSourceColumn_ = selectedSourceColumn_;
				rectangleSourceRow_ = selectedSourceRow_;
				rectangleFrameCount_ = selectedFrameCount_;
				rectangleVisualRole_ = selectedVisualRole_;
				rectangleVisualFlipX_ = selectedVisualFlipX_;
				rectangleVisualFlipY_ = selectedVisualFlipY_;
			}
			selectedObjectType_ = SelectedObjectType::None;
		}
		if (!isDraggingRectangle_ &&
			selectedTile_ != FallHoleTool &&
			selectedTile_ != CameraAreaTool &&
			(isRightTriggered ||
				(isLeftTriggered && selectedTile_ != SelectTool)))
		{
			isStrokeActive_ = true;
		}

		if (isDraggingRectangle_)
		{
			const int dragButton = isRectangleErase_
				? MOUSE_INPUT_RIGHT : MOUSE_INPUT_LEFT;
			if ((mouseInput & dragButton) != 0)
			{
				rectangleEndColumn_ = hoveredColumn_;
				rectangleEndRow_ = hoveredRow_;
			}
		}
		else if (selectedTile_ == CameraAreaTool)
		{
			if (isRightTriggered)
			{
				const int index = editorStageData_.FindCameraAreaAt(
					hoveredColumn_, hoveredRow_);
				if (editorStageData_.RemoveCameraArea(index))
				{
					history_.Commit(editorStageData_.GetStageData());
					selectedCameraAreaIndex_ = -1;
				}
			}
			else if (isLeftTriggered)
			{
				const bool isShiftPressed =
					inputManager.IsPressed(KEY_INPUT_LSHIFT) ||
					inputManager.IsPressed(KEY_INPUT_RSHIFT);
				const bool isControlPressed =
					inputManager.IsPressed(KEY_INPUT_LCONTROL) ||
					inputManager.IsPressed(KEY_INPUT_RCONTROL);
				const bool isReplacePressed = inputManager.IsPressed(KEY_INPUT_R) &&
					selectedCameraAreaIndex_ >= 0;
				const int index = editorStageData_.FindCameraAreaAt(
					hoveredColumn_, hoveredRow_);
				isReplacingCameraArea_ = isReplacePressed && !isShiftPressed;
				isAddingCameraAreaPart_ = isControlPressed &&
					selectedCameraAreaIndex_ >= 0 && !isReplacingCameraArea_;
				isDraggingCameraArea_ = isShiftPressed || isReplacingCameraArea_ ||
					isAddingCameraAreaPart_ || index < 0;
				if (!isDraggingCameraArea_)
				{
					selectedCameraAreaIndex_ = index;
				}
				cameraAreaStartColumn_ = cameraAreaEndColumn_ = hoveredColumn_;
				cameraAreaStartRow_ = cameraAreaEndRow_ = hoveredRow_;
			}
			else if (isDraggingCameraArea_ &&
				(mouseInput & MOUSE_INPUT_LEFT) != 0)
			{
				cameraAreaEndColumn_ = hoveredColumn_;
				cameraAreaEndRow_ = hoveredRow_;
			}
		}
		else if (selectedTile_ == FallHoleTool)
		{
			if (isRightTriggered)
			{
				if (editorStageData_.RemoveFallHoleAt(
					hoveredColumn_, hoveredRow_))
				{
					history_.Commit(editorStageData_.GetStageData());
				}
			}
			else if (isLeftTriggered)
			{
				fallHoleStartColumn_ = hoveredColumn_;
				fallHoleEndColumn_ = hoveredColumn_;
				fallHoleSurfaceRow_ = hoveredRow_;
				isDraggingFallHole_ = true;
			}
			else if (isDraggingFallHole_ &&
				(mouseInput & MOUSE_INPUT_LEFT) != 0)
			{
				fallHoleEndColumn_ = hoveredColumn_;
			}
		}
		// 押している間は連続配置できるため、ドラッグで地形を描ける。
		else if (isStrokeActive_ &&
			(mouseInput & MOUSE_INPUT_RIGHT) != 0)
		{
			isStrokeDirty_ = editorStageData_.Erase(
				hoveredColumn_,
				hoveredRow_
			) || isStrokeDirty_;

			if (selectedObjectType_ != SelectedObjectType::None &&
				selectedObjectColumn_ == hoveredColumn_ &&
				selectedObjectRow_ == hoveredRow_)
			{
				selectedObjectType_ = SelectedObjectType::None;
			}
		}
		else if (selectedTile_ == SelectTool && isLeftTriggered)
		{
			selectedObjectType_ = editorStageData_.HasEnemyAt(
				hoveredColumn_, hoveredRow_)
				? SelectedObjectType::Enemy
				: editorStageData_.HasGravityBlockAt(hoveredColumn_, hoveredRow_)
					? SelectedObjectType::GravityBlock
					: editorStageData_.HasSawAt(hoveredColumn_, hoveredRow_)
						? SelectedObjectType::Saw
						: editorStageData_.HasMidBossAt(hoveredColumn_, hoveredRow_)
							? SelectedObjectType::MidBoss
							: editorStageData_.HasBossAt(hoveredColumn_, hoveredRow_)
								? SelectedObjectType::Boss
								: SelectedObjectType::None;
			selectedObjectColumn_ = hoveredColumn_;
			selectedObjectRow_ = hoveredRow_;
			isDraggingObject_ = selectedObjectType_ != SelectedObjectType::None;
		}
		else if (isStrokeActive_ && selectedTile_ != SelectTool &&
			(mouseInput & MOUSE_INPUT_LEFT) != 0)
		{
			selectedObjectType_ = SelectedObjectType::None;
			const bool changed = selectedTile_ == SawTool
				? editorStageData_.PlaceSaw(hoveredColumn_, hoveredRow_)
				: selectedTile_ == MovingGravityBlockTool
					? editorStageData_.PlaceMovingGravityBlock(
						hoveredColumn_, hoveredRow_)
				: selectedTile_ == MidBossTool
					? editorStageData_.PlaceMidBoss(hoveredColumn_, hoveredRow_)
				: selectedTile_ == BossTool
					? editorStageData_.PlaceBoss(hoveredColumn_, hoveredRow_)
				: selectedTile_ == VisualBrushTool && renderer_
					? editorStageData_.PlaceVisual(hoveredColumn_, hoveredRow_,
						renderer_->GetSheetName(selectedSheet_),
						selectedSourceColumn_, selectedSourceRow_, selectedVisualRole_,
						selectedFrameCount_, selectedVisualFlipX_, selectedVisualFlipY_)
					: editorStageData_.Place(
						hoveredColumn_, hoveredRow_, selectedTile_);
			isStrokeDirty_ = changed || isStrokeDirty_;
			if (changed && selectedTile_ == VisualBrushTool)
			{
				RememberCurrentVisualTile();
			}
		}
	}

	if (isDraggingFallHole_ && isLeftReleased)
	{
		if (editorStageData_.AddFallHole(
			fallHoleStartColumn_,
			fallHoleEndColumn_,
			fallHoleSurfaceRow_
		))
		{
			history_.Commit(editorStageData_.GetStageData());
		}
		isDraggingFallHole_ = false;
	}
	if (isDraggingCameraArea_ && isLeftReleased)
	{
		bool changed = false;
		if (isReplacingCameraArea_)
		{
			changed = editorStageData_.ReplaceCameraArea(
				selectedCameraAreaIndex_, cameraAreaStartColumn_, cameraAreaStartRow_,
				cameraAreaEndColumn_, cameraAreaEndRow_);
		}
		else if (isAddingCameraAreaPart_)
		{
			changed = editorStageData_.AddCameraAreaPart(
				selectedCameraAreaIndex_, cameraAreaStartColumn_, cameraAreaStartRow_,
				cameraAreaEndColumn_, cameraAreaEndRow_);
			if (changed)
			{
				selectedCameraAreaIndex_ =
					editorStageData_.GetStageData().cameraAreaCount - 1;
			}
		}
		else
		{
			changed = editorStageData_.AddCameraArea(
				cameraAreaStartColumn_, cameraAreaStartRow_,
				cameraAreaEndColumn_, cameraAreaEndRow_);
			if (changed)
			{
				selectedCameraAreaIndex_ =
					editorStageData_.GetStageData().cameraAreaCount - 1;
			}
		}
		if (changed)
		{
			history_.Commit(editorStageData_.GetStageData());
		}
		isDraggingCameraArea_ = false;
		isReplacingCameraArea_ = false;
		isAddingCameraAreaPart_ = false;
	}
	if (isDraggingObject_ && isLeftReleased)
	{
		if (selectedObjectType_ != SelectedObjectType::None &&
			isHoveringStage_ &&
			(selectedObjectColumn_ != hoveredColumn_ ||
				selectedObjectRow_ != hoveredRow_))
		{
			bool moved = false;
			if (selectedObjectType_ == SelectedObjectType::Enemy)
			{
				moved = editorStageData_.MoveEnemy(selectedObjectColumn_,
					selectedObjectRow_, hoveredColumn_, hoveredRow_);
			}
			else if (selectedObjectType_ == SelectedObjectType::GravityBlock)
			{
				moved = editorStageData_.MoveGravityBlock(selectedObjectColumn_,
					selectedObjectRow_, hoveredColumn_, hoveredRow_);
			}
			else if (selectedObjectType_ == SelectedObjectType::Saw)
			{
				moved = editorStageData_.MoveSaw(selectedObjectColumn_,
					selectedObjectRow_, hoveredColumn_, hoveredRow_);
			}
			else if (selectedObjectType_ == SelectedObjectType::MidBoss)
			{
				moved = editorStageData_.MoveMidBoss(selectedObjectColumn_,
					selectedObjectRow_, hoveredColumn_, hoveredRow_);
			}
			else if (selectedObjectType_ == SelectedObjectType::Boss)
			{
				moved = editorStageData_.MoveBoss(selectedObjectColumn_,
					selectedObjectRow_, hoveredColumn_, hoveredRow_);
			}
			if (moved)
			{
				selectedObjectColumn_ = hoveredColumn_;
				selectedObjectRow_ = hoveredRow_;
				history_.Commit(editorStageData_.GetStageData());
			}
			else
			{
				moveMessageTimer_ = 120;
			}
		}
		isDraggingObject_ = false;
	}
	if (isDraggingRectangle_ &&
		(isRectangleErase_ ? isRightReleased : isLeftReleased))
	{
		if (isHoveringStage_)
		{
			rectangleEndColumn_ = hoveredColumn_;
			rectangleEndRow_ = hoveredRow_;
			bool changed = false;
			if (isRectangleErase_)
			{
				changed = editorStageData_.EraseRectangle(
					rectangleStartColumn_, rectangleStartRow_,
					rectangleEndColumn_, rectangleEndRow_);
			}
			else if (rectangleTile_ == VisualBrushTool)
			{
				changed = editorStageData_.FillVisualRectangle(
					rectangleStartColumn_, rectangleStartRow_,
					rectangleEndColumn_, rectangleEndRow_, rectangleSheetName_,
					rectangleSourceColumn_, rectangleSourceRow_, rectangleVisualRole_,
					rectangleFrameCount_, rectangleVisualFlipX_, rectangleVisualFlipY_);
			}
			else
			{
				changed = editorStageData_.FillRectangle(
					rectangleStartColumn_, rectangleStartRow_,
					rectangleEndColumn_, rectangleEndRow_, rectangleTile_);
			}
			if (changed)
			{
				if (rectangleTile_ == VisualBrushTool)
				{
					RememberCurrentVisualTile();
				}
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		isDraggingRectangle_ = false;
	}

	if ((mouseInput & (MOUSE_INPUT_LEFT | MOUSE_INPUT_RIGHT)) == 0)
	{
		FinishStroke();
	}

	if (selectedTile_ != FallHoleTool)
	{
		isDraggingFallHole_ = false;
	}
	if (selectedTile_ != CameraAreaTool)
	{
		isDraggingCameraArea_ = false;
		isReplacingCameraArea_ = false;
		isAddingCameraAreaPart_ = false;
	}
	else if (selectedCameraAreaIndex_ >= 0)
	{
		if (inputManager.IsTriggered(KEY_INPUT_M))
		{
			if (editorStageData_.CycleCameraAreaMode(selectedCameraAreaIndex_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_DELETE))
		{
			if (editorStageData_.RemoveCameraArea(selectedCameraAreaIndex_))
			{
				history_.Commit(editorStageData_.GetStageData());
				selectedCameraAreaIndex_ = -1;
			}
		}
	}

	// 選択中の敵に対して、キーを押した瞬間だけHPを変更する。
	if (selectedObjectType_ == SelectedObjectType::Enemy)
	{
		if (inputManager.IsTriggered(KEY_INPUT_Z))
		{
			FinishStroke();
			if (editorStageData_.ChangeEnemyHitPointAt(
				selectedObjectColumn_,
				selectedObjectRow_,
				-1
			))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_X))
		{
			FinishStroke();
			if (editorStageData_.ChangeEnemyHitPointAt(
				selectedObjectColumn_,
				selectedObjectRow_,
				1
			))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_G))
		{
			FinishStroke();
			if (editorStageData_.ToggleEnemyGravityResponseAt(
				selectedObjectColumn_, selectedObjectRow_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_R))
		{
			FinishStroke();
			if (editorStageData_.ToggleEnemyFacingAt(
				selectedObjectColumn_, selectedObjectRow_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}

		if (!editorStageData_.HasEnemyAt(
			selectedObjectColumn_,
			selectedObjectRow_))
		{
			selectedObjectType_ = SelectedObjectType::None;
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::GravityBlock)
	{
		if (inputManager.IsTriggered(KEY_INPUT_R))
		{
			FinishStroke();
			if (editorStageData_.ToggleGravityBlockMotionAt(
				selectedObjectColumn_, selectedObjectRow_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		if (!editorStageData_.HasGravityBlockAt(
			selectedObjectColumn_, selectedObjectRow_))
		{
			selectedObjectType_ = SelectedObjectType::None;
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::Saw)
	{
		if (inputManager.IsTriggered(KEY_INPUT_X) ||
			inputManager.IsTriggered(KEY_INPUT_Z))
		{
			FinishStroke();
			if (editorStageData_.FlipSawAt(selectedObjectColumn_,
				selectedObjectRow_, inputManager.IsTriggered(KEY_INPUT_X)))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		if (!editorStageData_.HasSawAt(selectedObjectColumn_, selectedObjectRow_))
		{
			selectedObjectType_ = SelectedObjectType::None;
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::MidBoss)
	{
		if (inputManager.IsTriggered(KEY_INPUT_Z) ||
			inputManager.IsTriggered(KEY_INPUT_X))
		{
			FinishStroke();
			const int amount = inputManager.IsTriggered(KEY_INPUT_Z) ? -1 : 1;
			if (editorStageData_.ChangeBossHitPointAt(
				selectedObjectColumn_, selectedObjectRow_, amount))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_R))
		{
			FinishStroke();
			if (editorStageData_.ToggleMidBossFacingAt(
				selectedObjectColumn_, selectedObjectRow_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		if (!editorStageData_.HasMidBossAt(
			selectedObjectColumn_, selectedObjectRow_))
		{
			selectedObjectType_ = SelectedObjectType::None;
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::Boss)
	{
		if (inputManager.IsTriggered(KEY_INPUT_Z) ||
			inputManager.IsTriggered(KEY_INPUT_X))
		{
			FinishStroke();
			const int amount = inputManager.IsTriggered(KEY_INPUT_Z) ? -1 : 1;
			if (editorStageData_.ChangeBossHitPointAt(
				selectedObjectColumn_, selectedObjectRow_, amount))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		else if (inputManager.IsTriggered(KEY_INPUT_R))
		{
			FinishStroke();
			if (editorStageData_.ToggleBossFacingAt(
				selectedObjectColumn_, selectedObjectRow_))
			{
				history_.Commit(editorStageData_.GetStageData());
			}
		}
		if (!editorStageData_.HasBossAt(
			selectedObjectColumn_, selectedObjectRow_))
		{
			selectedObjectType_ = SelectedObjectType::None;
		}
	}

	previousMouseInput_ = mouseInput;
	if (showValidation_)
	{
		RefreshValidation();
	}
}

// 編集中のStageDataを点検し、選択中の指摘番号を有効範囲に保つ。
void StageEditor::RefreshValidation()
{
	validationIssues_ = StageValidator::Validate(editorStageData_.GetStageData());
	if (selectedIssueIndex_ >= static_cast<int>(validationIssues_.size()))
	{
		selectedIssueIndex_ = 0;
	}
}

// ブラウザー内のクリックはステージへの描画として扱わない。
void StageEditor::SelectImageSheet(int index)
{
	if (selectedSheet_ == index)
	{
		return;
	}
	selectedSheet_ = index;
	selectedSourceColumn_ = 0;
	selectedSourceRow_ = 0;
	selectedFrameCount_ = 1;
	selectedVisualFlipX_ = false;
	selectedVisualFlipY_ = false;
	hasSelectedSource_ = false;
	if (selectedTile_ == VisualBrushTool)
	{
		selectedTile_ = StageData::FloorTile;
	}
}

void StageEditor::RememberCurrentVisualTile()
{
	if (!renderer_ || !hasSelectedSource_ || selectedSheet_ < 0)
	{
		return;
	}

	const RecentVisualTile current{
		selectedSheet_, selectedSourceColumn_, selectedSourceRow_,
		selectedFrameCount_, selectedVisualRole_,
		selectedVisualFlipX_, selectedVisualFlipY_
	};
	int existingIndex = -1;
	for (int i = 0; i < recentVisualTileCount_; ++i)
	{
		const RecentVisualTile& recent = recentVisualTiles_[i];
		if (recent.sheetIndex == current.sheetIndex &&
			recent.sourceColumn == current.sourceColumn &&
			recent.sourceRow == current.sourceRow &&
			recent.frameCount == current.frameCount &&
			recent.role == current.role &&
			recent.flipX == current.flipX && recent.flipY == current.flipY)
		{
			existingIndex = i;
			break;
		}
	}

	const int lastIndex = existingIndex >= 0
		? existingIndex
		: (std::min)(recentVisualTileCount_, MaxRecentVisualTiles - 1);
	for (int i = lastIndex; i > 0; --i)
	{
		recentVisualTiles_[i] = recentVisualTiles_[i - 1];
	}
	recentVisualTiles_[0] = current;
	if (existingIndex < 0 && recentVisualTileCount_ < MaxRecentVisualTiles)
	{
		++recentVisualTileCount_;
	}
}

void StageEditor::SelectRecentVisualTile(int index)
{
	if (!renderer_ || index < 0 || index >= recentVisualTileCount_)
	{
		return;
	}

	const RecentVisualTile& recent = recentVisualTiles_[index];
	if (recent.sheetIndex < 0 || recent.sheetIndex >= renderer_->GetSheetCount())
	{
		return;
	}
	selectedSheet_ = recent.sheetIndex;
	selectedSourceColumn_ = recent.sourceColumn;
	selectedSourceRow_ = recent.sourceRow;
	selectedFrameCount_ = recent.frameCount;
	selectedVisualRole_ = recent.role;
	selectedVisualFlipX_ = recent.flipX;
	selectedVisualFlipY_ = recent.flipY;
	hasSelectedSource_ = true;
	selectedTile_ = VisualBrushTool;
}

void StageEditor::UpdateImageBrowser(const InputManager& inputManager)
{
	const int count = renderer_ ? renderer_->GetSheetCount() : 0;
	if (count == 0)
	{
		previousMouseInput_ = GetMouseInput();
		return;
	}
	if (inputManager.IsTriggered(KEY_INPUT_UP))
	{
		SelectImageSheet((std::max)(0, selectedSheet_ - 1));
	}
	if (inputManager.IsTriggered(KEY_INPUT_DOWN))
	{
		SelectImageSheet((std::min)(count - 1, selectedSheet_ + 1));
	}
	if (inputManager.IsTriggered(KEY_INPUT_LEFT))
	{
		SelectImageSheet((std::max)(0, selectedSheet_ - BrowserPageSize));
	}
	if (inputManager.IsTriggered(KEY_INPUT_RIGHT))
	{
		SelectImageSheet((std::min)(count - 1,
			selectedSheet_ + BrowserPageSize));
	}
	if (hasSelectedSource_ && inputManager.IsTriggered(KEY_INPUT_X))
	{
		selectedVisualFlipX_ = !selectedVisualFlipX_;
	}
	if (hasSelectedSource_ && inputManager.IsTriggered(KEY_INPUT_Y))
	{
		selectedVisualFlipY_ = !selectedVisualFlipY_;
	}

	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);
	const int mouseInput = GetMouseInput();
	const bool clicked = (mouseInput & MOUSE_INPUT_LEFT) != 0 &&
		(previousMouseInput_ & MOUSE_INPUT_LEFT) == 0;
	previousMouseInput_ = mouseInput;
	if (!clicked)
	{
		return;
	}

	const int page = selectedSheet_ / BrowserPageSize;
	if (mouseY >= BrowserRecentTop &&
		mouseY < BrowserRecentTop + BrowserRecentSize &&
		mouseX >= BrowserRecentLeft)
	{
		const int slotWidth = BrowserRecentSize + BrowserRecentGap;
		const int slot = (mouseX - BrowserRecentLeft) / slotWidth;
		const int slotOffset = (mouseX - BrowserRecentLeft) % slotWidth;
		if (slotOffset < BrowserRecentSize && slot < recentVisualTileCount_)
		{
			SelectRecentVisualTile(slot);
		}
		return;
	}
	if (mouseX >= 20 && mouseX < 370 && mouseY >= BrowserListTop &&
		mouseY < BrowserListTop + BrowserPageSize * 26)
	{
		const int index = page * BrowserPageSize +
			(mouseY - BrowserListTop) / 26;
		if (index < count)
		{
			SelectImageSheet(index);
		}
		return;
	}
	if (mouseY >= 160 && mouseY < 195 && mouseX >= 405 && mouseX < 1205)
	{
		const char roles[] = { StageData::FloorTile,
			StageData::WallTile, StageData::BlockTile, StageData::EmptyTile };
		selectedVisualRole_ = roles[(mouseX - 405) / 200];
		return;
	}

	const int columns = renderer_->GetSheetColumns(selectedSheet_);
	const int rows = renderer_->GetSheetRows(selectedSheet_);
	if (columns <= 0 || rows <= 0)
	{
		return;
	}
	const int cellSize = (std::min)(64,
		(std::min)(BrowserPreviewWidth / columns,
			BrowserPreviewHeight / rows));
	if (cellSize <= 0)
	{
		return;
	}
	if (mouseX >= BrowserPreviewLeft &&
		mouseX < BrowserPreviewLeft + columns * cellSize &&
		mouseY >= BrowserPreviewTop &&
		mouseY < BrowserPreviewTop + rows * cellSize)
	{
		if (renderer_->IsAnimatedSheet(selectedSheet_))
		{
			selectedSourceColumn_ = 0;
			selectedSourceRow_ = 0;
			selectedFrameCount_ = (std::min)(columns * rows, 255);
		}
		else
		{
			selectedSourceColumn_ = (mouseX - BrowserPreviewLeft) / cellSize;
			selectedSourceRow_ = (mouseY - BrowserPreviewTop) / cellSize;
			selectedFrameCount_ = 1;
		}
		hasSelectedSource_ = true;
		selectedTile_ = VisualBrushTool;
	}
}

void StageEditor::DrawImageBrowser() const
{
	DrawBox(0, EditorToolbarHeight, GameConfig::ScreenWidth,
		GameConfig::ScreenHeight, GetColor(13, 18, 27), TRUE);
	const int white = GetColor(230, 235, 240);
	const int accent = GetColor(110, 220, 255);
	UiFont::DrawString(20, 118, L"タイル画像　I / ESC：閉じる", accent);
	UiFont::DrawString(20, 148, L"上下：画像選択　左右：ページ", white);
	UiFont::DrawString(405, 124, L"最近使用", white);
	const int count = renderer_ ? renderer_->GetSheetCount() : 0;
	if (count == 0)
	{
		UiFont::DrawString(20, 200,
			L"タイル画像フォルダーにPNG画像がありません",
			GetColor(255, 120, 120));
		return;
	}
	const int page = selectedSheet_ / BrowserPageSize;
	for (int i = 0; i < recentVisualTileCount_; ++i)
	{
		const RecentVisualTile& recent = recentVisualTiles_[i];
		const int left = BrowserRecentLeft +
			i * (BrowserRecentSize + BrowserRecentGap);
		const int right = left + BrowserRecentSize;
		const int bottom = BrowserRecentTop + BrowserRecentSize;
		const bool isSelected = hasSelectedSource_ &&
			selectedSheet_ == recent.sheetIndex &&
			selectedSourceColumn_ == recent.sourceColumn &&
			selectedSourceRow_ == recent.sourceRow &&
			selectedFrameCount_ == recent.frameCount &&
			selectedVisualRole_ == recent.role &&
			selectedVisualFlipX_ == recent.flipX &&
			selectedVisualFlipY_ == recent.flipY;
		DrawBox(left - 1, BrowserRecentTop - 1, right + 1, bottom + 1,
			isSelected ? accent : GetColor(75, 100, 120), FALSE);
		renderer_->DrawSheetCell(recent.sheetIndex, recent.sourceColumn,
			recent.sourceRow, recent.frameCount,
			left, BrowserRecentTop, right, bottom,
			recent.flipX, recent.flipY);
		const int roleColor = recent.role == StageData::EmptyTile
			? GetColor(190, 100, 255)
			: recent.role == StageData::WallTile
			? GetColor(100, 170, 255)
			: recent.role == StageData::BlockTile
			? GetColor(210, 210, 210)
			: GetColor(80, 240, 200);
		DrawLine(left, bottom - 2, right, bottom - 2, roleColor, 3);
	}
	UiFont::DrawFormatString(20, 600, white, L"ページ %d / %d",
		page + 1, (count + BrowserPageSize - 1) / BrowserPageSize);
	for (int slot = 0; slot < BrowserPageSize; ++slot)
	{
		const int index = page * BrowserPageSize + slot;
		if (index >= count)
		{
			break;
		}
		const int top = BrowserListTop + slot * 26;
		if (index == selectedSheet_)
		{
			DrawBox(18, top, 378, top + 24, GetColor(40, 80, 105), TRUE);
		}
		std::wstring wideName = ToWide(renderer_->GetSheetName(index));
		if (wideName.size() > 40)
		{
			wideName = wideName.substr(0, 37) + L"...";
		}
		UiFont::DrawString(24, top + 3, wideName.c_str(),
			index == selectedSheet_ ? accent : white);
	}

	const wchar_t* labels[] = { L"床", L"壁", L"ブロック", L"背景" };
	const char roles[] = { StageData::FloorTile,
		StageData::WallTile, StageData::BlockTile, StageData::EmptyTile };
	for (int i = 0; i < 4; ++i)
	{
		const int left = 405 + i * 200;
		const int border = selectedVisualRole_ == roles[i] ? accent
			: GetColor(75, 100, 120);
		DrawBox(left, 160, left + 188, 195, GetColor(27, 40, 55), TRUE);
		DrawBox(left, 160, left + 188, 195, border, FALSE);
		UiFont::DrawString(left + 10, 169, labels[i], border);
	}
	UiFont::DrawString(405, 204, renderer_->IsAnimatedSheet(selectedSheet_)
		? L"シートを選択：アニメ　X：左右反転　Y：上下反転　I：配置"
		: L"セルを選択　X：左右反転　Y：上下反転　I：配置", white);

	const int columns = renderer_->GetSheetColumns(selectedSheet_);
	const int rows = renderer_->GetSheetRows(selectedSheet_);
	if (columns <= 0 || rows <= 0)
	{
		return;
	}
	const int cellSize = (std::min)(64,
		(std::min)(BrowserPreviewWidth / columns,
			BrowserPreviewHeight / rows));
	if (cellSize <= 0)
	{
		return;
	}
	const int right = BrowserPreviewLeft + columns * cellSize;
	const int bottom = BrowserPreviewTop + rows * cellSize;
	renderer_->DrawSheet(selectedSheet_, BrowserPreviewLeft, BrowserPreviewTop,
		right, bottom);
	for (int column = 0; column <= columns; ++column)
	{
		const int x = BrowserPreviewLeft + column * cellSize;
		DrawLine(x, BrowserPreviewTop, x, bottom, GetColor(80, 100, 120));
	}
	for (int row = 0; row <= rows; ++row)
	{
		const int y = BrowserPreviewTop + row * cellSize;
		DrawLine(BrowserPreviewLeft, y, right, y, GetColor(80, 100, 120));
	}
	if (hasSelectedSource_)
	{
		if (selectedFrameCount_ > 1)
		{
			DrawBox(BrowserPreviewLeft, BrowserPreviewTop, right, bottom,
				GetColor(255, 230, 70), FALSE);
		}
		else
		{
			const int left = BrowserPreviewLeft + selectedSourceColumn_ * cellSize;
			const int top = BrowserPreviewTop + selectedSourceRow_ * cellSize;
			DrawBox(left, top, left + cellSize, top + cellSize,
				GetColor(255, 230, 70), FALSE);
		}
	}
	UiFont::DrawFormatString(405, 654, white,
		L"セル：%d, %d　コマ数：%d　左右反転：%s　上下反転：%s",
		selectedSourceColumn_, selectedSourceRow_, selectedFrameCount_,
		selectedVisualFlipX_ ? L"あり" : L"なし",
		selectedVisualFlipY_ ? L"あり" : L"なし");
	if (hasSelectedSource_)
	{
		UiFont::DrawString(BrowserSelectedPreviewLeft, 204, L"選択中", white);
		renderer_->DrawSheetCell(selectedSheet_, selectedSourceColumn_,
			selectedSourceRow_, selectedFrameCount_,
			BrowserSelectedPreviewLeft, BrowserSelectedPreviewTop,
			BrowserSelectedPreviewLeft + BrowserSelectedPreviewSize,
			BrowserSelectedPreviewTop + BrowserSelectedPreviewSize,
			selectedVisualFlipX_, selectedVisualFlipY_);
		DrawBox(
			BrowserSelectedPreviewLeft - 1,
			BrowserSelectedPreviewTop - 1,
			BrowserSelectedPreviewLeft + BrowserSelectedPreviewSize + 1,
			BrowserSelectedPreviewTop + BrowserSelectedPreviewSize + 1,
			accent, FALSE);
	}
}

// 位置のある指摘なら、そのマスが見える場所へカメラを動かす。
void StageEditor::FocusSelectedIssue()
{
	const StageValidationIssue& issue = validationIssues_[selectedIssueIndex_];
	if (issue.column < 0 || issue.row < 0)
	{
		return;
	}
	cameraX_ = issue.column * StageData::TileSize - EditorViewportWidth / 2;
	cameraY_ = issue.row * StageData::TileSize -
		(GameConfig::ScreenHeight - EditorToolbarHeight) / 2;
	ClampCamera();
}

// 連続したドラッグを1つの履歴として確定する。
void StageEditor::FinishStroke()
{
	if (isStrokeActive_ && isStrokeDirty_)
	{
		history_.Commit(editorStageData_.GetStageData());
	}

	isStrokeActive_ = false;
	isStrokeDirty_ = false;
}

const StageData& StageEditor::PrepareForPreview()
{
	FinishStroke();
	// 穴とカメラエリアはドラッグ確定前なら変更せずに中止する。
	isDraggingRectangle_ = false;
	isDraggingFallHole_ = false;
	isDraggingCameraArea_ = false;
	isDraggingObject_ = false;
	previousMouseInput_ = GetMouseInput();
	previewMessageTimer_ = 0;
	return editorStageData_.GetStageData();
}

bool StageEditor::TryPrepareCursorPreview(float playerWidth, float playerHeight,
	StageData& previewStage)
{
	const StageData& stageData = PrepareForPreview();
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);
	const int worldX = mouseX + cameraX_;
	const int worldY = mouseY - EditorToolbarHeight + cameraY_;
	if (mouseX < 0 || mouseX >= EditorViewportWidth ||
		mouseY < EditorToolbarHeight || mouseY >= GameConfig::ScreenHeight ||
		worldX < 0 || worldX >= worldWidth_ ||
		worldY < 0 || worldY >= worldHeight_)
	{
		previewMessageTimer_ = 120;
		return false;
	}

	Vector2 startPosition{};
	if (!StagePreviewSpawn::TryGetPosition(stageData,
		worldX / StageData::TileSize, worldY / StageData::TileSize,
		playerWidth, playerHeight, startPosition))
	{
		previewMessageTimer_ = 120;
		return false;
	}

	// 保存対象とは別のコピーにだけ開始地点を設定する。
	previewStage = stageData;
	previewStage.playerStartPosition = startPosition;
	previewStage.hasPlayerStartPosition = true;
	return true;
}

void StageEditor::ResumeAfterPreview()
{
	// プレビュー中に変わったマウス入力を新しいクリックと誤認しない。
	previousMouseInput_ = GetMouseInput();
	isHoveringStage_ = false;
	isDraggingRectangle_ = false;
	isDraggingObject_ = false;
}

// ステージの地形を色分けした矩形で表示し、その上へグリッドを重ねる。
void StageEditor::Draw() const
{
	const StageData& stageData = editorStageData_.GetStageData();
	const int backgroundColor = GetColor(16, 20, 30);
	const int blockColor = GetColor(60, 105, 125);
	const int floorColor = GetColor(65, 135, 165);
	const int wallColor = GetColor(95, 120, 145);
	const int tileBorderColor = GetColor(130, 190, 210);
	const int invisibleColor = GetColor(205, 80, 235);
	const int invisibleBorderColor = GetColor(255, 155, 255);
	const int gridColor = GetColor(45, 65, 80);
	const int textColor = GetColor(235, 240, 245);
	const int accentColor = GetColor(100, 220, 255);

	DrawBox(
		0,
		0,
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		backgroundColor,
		TRUE
	);
	SetDrawArea(0, EditorToolbarHeight,
		EditorViewportWidth, GameConfig::ScreenHeight);

	const int firstColumn = cameraX_ / StageData::TileSize;
	const int firstRow = cameraY_ / StageData::TileSize;
	const int lastColumn = ClampInt(
		(cameraX_ + EditorViewportWidth) / StageData::TileSize,
		0,
		StageData::TileColumns - 1
	);
	const int lastRow = ClampInt(
		(cameraY_ + GameConfig::ScreenHeight - EditorToolbarHeight) /
		StageData::TileSize,
		0,
		stageData.tileRowCount - 1
	);

	for (int row = firstRow; row <= lastRow; ++row)
	{
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			const int left = column * StageData::TileSize - cameraX_;
			const int top =
				row * StageData::TileSize - cameraY_ + EditorToolbarHeight;
			const int right = left + StageData::TileSize;
			const int bottom = top + StageData::TileSize;
			if (renderer_)
			{
				renderer_->DrawVisualTile(stageData,
					stageData.backgroundVisuals[row][column],
					left, top, right, bottom);
			}
			const char tile = stageData.tiles[row][column];
			if (!StageData::IsSolidTile(tile))
			{
				continue;
			}
			if (tile == StageData::InvisibleBlockTile)
			{
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 75);
				DrawBox(left, top, right, bottom, invisibleColor, TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
				DrawBox(left, top, right, bottom, invisibleBorderColor, FALSE);
				DrawLine(left, top, right, bottom, invisibleBorderColor);
				DrawLine(right, top, left, bottom, invisibleBorderColor);
				continue;
			}
			if (renderer_ && renderer_->DrawVisualTile(stageData,
				stageData.solidVisuals[row][column],
				left, top, right, bottom))
			{
				continue;
			}

			int tileColor = blockColor;
			if (tile == StageData::FloorTile)
			{
				tileColor = floorColor;
			}
			else if (tile == StageData::WallTile)
			{
				tileColor = wallColor;
			}

			DrawBox(left, top, right, bottom, tileColor, TRUE);
			DrawBox(left, top, right, bottom, tileBorderColor, FALSE);
		}
	}

	// タイルの境界が分かるように、画面全体へグリッドを描画する。
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);

	for (int worldX = firstColumn * StageData::TileSize;
		worldX <= cameraX_ + EditorViewportWidth;
		worldX += StageData::TileSize)
	{
		const int screenX = worldX - cameraX_;
		DrawLine(
			screenX,
			EditorToolbarHeight,
			screenX,
			GameConfig::ScreenHeight,
			gridColor
		);
	}

	for (int worldY = firstRow * StageData::TileSize;
		worldY <= cameraY_ + GameConfig::ScreenHeight - EditorToolbarHeight;
		worldY += StageData::TileSize)
	{
		const int screenY =
			worldY - cameraY_ + EditorToolbarHeight;
		DrawLine(0, screenY, EditorViewportWidth, screenY, gridColor);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// 確定前は範囲だけを見せ、編集データには触れない。
	if (isDraggingRectangle_)
	{
		const int left = (std::max)(0,
			(std::min)(rectangleStartColumn_, rectangleEndColumn_) *
			StageData::TileSize - cameraX_);
		const int top = (std::max)(EditorToolbarHeight,
			(std::min)(rectangleStartRow_, rectangleEndRow_) *
			StageData::TileSize - cameraY_ + EditorToolbarHeight);
		const int right = (std::min)(EditorViewportWidth,
			((std::max)(rectangleStartColumn_, rectangleEndColumn_) + 1) *
			StageData::TileSize - cameraX_);
		const int bottom = (std::min)(GameConfig::ScreenHeight,
			((std::max)(rectangleStartRow_, rectangleEndRow_) + 1) *
			StageData::TileSize - cameraY_ + EditorToolbarHeight);
		if (left < right && top < bottom)
		{
			const int previewColor = isRectangleErase_
				? GetColor(255, 100, 110) : GetColor(255, 230, 90);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 65);
			DrawBox(left, top, right, bottom, previewColor, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawBox(left, top, right, bottom, previewColor, FALSE);
		}
	}

	// カメラエリアは半透明で重ね、登録順と現在選択中の範囲を表示する。
	if (selectedTile_ == CameraAreaTool)
	{
		int selectedGroupId = -1;
		if (selectedCameraAreaIndex_ >= 0 &&
			selectedCameraAreaIndex_ < stageData.cameraAreaCount)
		{
			selectedGroupId =
				stageData.cameraAreas[selectedCameraAreaIndex_].groupId;
		}
		for (int i = 0; i < stageData.cameraAreaCount; ++i)
		{
			const CameraArea& area = stageData.cameraAreas[i];
			const int left = area.left - cameraX_;
			const int top = area.top - cameraY_ + EditorToolbarHeight;
			const int right = area.right - cameraX_;
			const int bottom = area.bottom - cameraY_ + EditorToolbarHeight;
			const bool isSelectedGroup = i == selectedCameraAreaIndex_ ||
				(selectedGroupId >= 0 && area.groupId == selectedGroupId);
			const int color = isSelectedGroup
				? GetColor(255, 230, 80) : GetColor(75, 210, 255);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 35);
			DrawBox(left, top, right, bottom, color, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawBox(left, top, right, bottom, color, FALSE);
			if (top >= EditorToolbarHeight && top < GameConfig::ScreenHeight &&
				left >= 0 && left < EditorViewportWidth)
			{
				UiFont::DrawFormatString(left + 4, top + 4, color, L"G%d：%s",
					area.groupId >= 0 ? area.groupId + 1 : i + 1,
					GetCameraAreaModeText(area.mode));
			}
		}

		if (isDraggingCameraArea_)
		{
			const int left = (std::min)(cameraAreaStartColumn_, cameraAreaEndColumn_)
				* StageData::TileSize - cameraX_;
			const int top = (std::min)(cameraAreaStartRow_, cameraAreaEndRow_)
				* StageData::TileSize - cameraY_ + EditorToolbarHeight;
			const int right = ((std::max)(cameraAreaStartColumn_, cameraAreaEndColumn_) + 1)
				* StageData::TileSize - cameraX_;
			const int bottom = ((std::max)(cameraAreaStartRow_, cameraAreaEndRow_) + 1)
				* StageData::TileSize - cameraY_ + EditorToolbarHeight;
			DrawBox(left, top, right, bottom, GetColor(255, 255, 100), FALSE);
		}
	}

	// 登録済みの穴を、実際にくり抜かれる矩形へ表示する。
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
	for (int i = 0; i < stageData.fallHoleCount; ++i)
	{
		const FallHole& hole = stageData.fallHoles[i];
		const int left = hole.left - cameraX_;
		const int right = hole.right - cameraX_;
		if (hole.IsLegacyBoundaryHole())
		{
			const int ceilingTop = EditorToolbarHeight - cameraY_;
			const int ceilingBottom = ceilingTop + StageData::TileSize * 2;
			const int floorTop = stageData.GetPixelHeight() -
				StageData::TileSize * 2 - cameraY_ + EditorToolbarHeight;
			const int floorBottom = floorTop + StageData::TileSize * 2;
			DrawBox(left, ceilingTop, right, ceilingBottom,
				GetColor(180, 45, 65), TRUE);
			DrawBox(left, floorTop, right, floorBottom,
				GetColor(180, 45, 65), TRUE);
			continue;
		}

		DrawBox(left, hole.top - cameraY_ + EditorToolbarHeight,
			right, hole.bottom - cameraY_ + EditorToolbarHeight,
			GetColor(180, 45, 65), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// ドラッグ中は、選択した床・天井の行へ確定予定範囲を表示する。
	if (isDraggingFallHole_)
	{
		int firstColumn = fallHoleStartColumn_;
		int lastColumn = fallHoleEndColumn_;
		if (firstColumn > lastColumn)
		{
			const int temporary = firstColumn;
			firstColumn = lastColumn;
			lastColumn = temporary;
		}

		const int previewLeft =
			firstColumn * StageData::TileSize - cameraX_;
		const int previewRight =
			(lastColumn + 1) * StageData::TileSize - cameraX_;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
		DrawBox(
			previewLeft,
			fallHoleSurfaceRow_ * StageData::TileSize - cameraY_ +
				EditorToolbarHeight,
			previewRight,
			(fallHoleSurfaceRow_ + 1) * StageData::TileSize - cameraY_ +
				EditorToolbarHeight,
			GetColor(255, 70, 90),
			TRUE
		);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// 地形の上へ、プレイヤー開始地点とゴールを重ねて表示する。
	if (stageData.hasPlayerStartPosition)
	{
		DrawObjectMarker(
			stageData.playerStartPosition,
			L"P",
			GetColor(80, 190, 255)
		);
	}

	if (stageData.hasGoalPosition)
	{
		DrawObjectMarker(
			stageData.goalPosition,
			L"G",
			GetColor(255, 220, 70)
		);
	}

	for (int i = 0; i < stageData.checkpointCount; ++i)
	{
		DrawObjectMarker(
			stageData.checkpoints[i],
			L"K",
			GetColor(80, 235, 255));
	}

	// 重力ブロックは保存座標から元の配置マスを求め、動作の種類を表示する。
	for (int i = 0; i < stageData.gravityBlockSpawnCount; ++i)
	{
		constexpr int blockSize = 64;
		const GravityBlockSpawnData& spawnData =
			stageData.gravityBlockSpawns[i];
		const int column =
			(static_cast<int>(spawnData.position.x) + blockSize / 2) /
			StageData::TileSize;
		const int row =
			spawnData.surface == GravityBlockSpawnSurface::Ceiling
				? static_cast<int>(spawnData.position.y) / StageData::TileSize
				: (static_cast<int>(spawnData.position.y) + blockSize - 1) /
					StageData::TileSize;

		const wchar_t* markerText = spawnData.motion == GravityBlockMotion::Vertical
			? L"縦"
			: spawnData.motion == GravityBlockMotion::Horizontal
				? L"横" : L"B";
		const int markerColor = spawnData.motion == GravityBlockMotion::Gravity
			? GetColor(100, 210, 220)
			: GetColor(255, 150, 70);
		DrawObjectMarker(
			{
				static_cast<float>(column * StageData::TileSize),
				static_cast<float>(row * StageData::TileSize)
			},
			markerText,
			markerColor
		);
	}

	// 敵も保存座標と種類から、対応するマスと記号を表示する。
	for (int i = 0; i < stageData.enemySpawnCount; ++i)
	{
		const EnemySpawnData& spawnData = stageData.enemySpawns[i];
		const int enemySize = spawnData.type == EnemyType::Heavy ? 56
			: spawnData.type == EnemyType::Charger ? 80
			: spawnData.type == EnemyType::Mortar ? 60 : 48;
		const int column =
			(static_cast<int>(spawnData.position.x) + enemySize / 2) /
			StageData::TileSize;
		const int row =
			spawnData.surface == EnemySpawnSurface::Ceiling
				? static_cast<int>(spawnData.position.y) / StageData::TileSize
				: (static_cast<int>(spawnData.position.y) + enemySize - 1) /
					StageData::TileSize;

		const wchar_t* markerText = L"E";
		int markerColor = GetColor(255, 100, 100);
		if (spawnData.type == EnemyType::Heavy)
		{
			markerText = L"H";
			markerColor = GetColor(190, 110, 255);
		}
		else if (spawnData.type == EnemyType::Shooter)
		{
			markerText = L"S";
			markerColor = GetColor(255, 160, 70);
		}
		else if (spawnData.type == EnemyType::UpShooter)
		{
			markerText = L"U";
			markerColor = GetColor(255, 100, 220);
		}
		else if (spawnData.type == EnemyType::Charger)
		{
			markerText = spawnData.facing == EnemySpawnFacing::Right
				? L"R>" : L"<R";
			markerColor = GetColor(70, 190, 255);
		}
		else if (spawnData.type == EnemyType::Mortar)
		{
			markerText = L"M";
			markerColor = GetColor(100, 240, 255);
		}
		if (!spawnData.isAffectedByGravityReversal)
		{
			markerColor = GetColor(100, 220, 255);
		}

		DrawObjectMarker(
			{
				static_cast<float>(column * StageData::TileSize),
				static_cast<float>(row * StageData::TileSize)
			},
			markerText,
			markerColor
		);
	}

	if (stageData.hasMidBossSpawn)
	{
		constexpr int bossSize = MidBossSpawnData::Size;
		const MidBossSpawnData& spawn = stageData.midBossSpawn;
		const int column =
			(static_cast<int>(spawn.position.x) + bossSize / 2) /
			StageData::TileSize;
		const int row = spawn.surface == EnemySpawnSurface::Ceiling
			? static_cast<int>(spawn.position.y) / StageData::TileSize
			: (static_cast<int>(spawn.position.y) + bossSize - 1) /
				StageData::TileSize;
		DrawObjectMarker(
			{ static_cast<float>(column * StageData::TileSize),
				static_cast<float>(row * StageData::TileSize) },
			spawn.facing == EnemySpawnFacing::Right ? L"O>" : L"<O",
			GetColor(70, 235, 150));
		UiFont::DrawFormatString(column * StageData::TileSize - cameraX_,
			row * StageData::TileSize - cameraY_ + EditorToolbarHeight - 16,
			GetColor(130, 255, 190), L"グループ%d", spawn.cameraGroupId);
	}

	if (stageData.hasBossSpawn)
	{
		const BossSpawnData& spawn = stageData.bossSpawn;
		const int column =
			(static_cast<int>(spawn.position.x) + BossSpawnData::Width / 2) /
			StageData::TileSize;
		const int row =
			(static_cast<int>(spawn.position.y) + BossSpawnData::Height - 1) /
			StageData::TileSize;
		DrawObjectMarker(
			{ static_cast<float>(column * StageData::TileSize),
				static_cast<float>(row * StageData::TileSize) },
			spawn.facing == EnemySpawnFacing::Right ? L"B>" : L"<B",
			GetColor(255, 145, 65));
		UiFont::DrawFormatString(column * StageData::TileSize - cameraX_,
			row * StageData::TileSize - cameraY_ + EditorToolbarHeight - 16,
			GetColor(255, 195, 110), L"グループ%d", spawn.cameraGroupId);
	}

	// 選択中の配置物を囲み、ドラッグ中は移動先にも枠を出す。
	for (int i = 0; i < stageData.sawSpawnCount; ++i)
	{
		const SawSpawnData& saw = stageData.sawSpawns[i];
		const Vector2& center = saw.position;
		DrawObjectMarker(
			{ center.x - StageData::TileSize * 0.5f,
				center.y - StageData::TileSize * 0.5f },
			L"W", GetColor(255, 115, 70));
		UiFont::DrawFormatString(static_cast<int>(center.x) - cameraX_ + 4,
			static_cast<int>(center.y) - cameraY_ + EditorToolbarHeight - 16,
			GetColor(255, 215, 145), L"%s%s",
			saw.flipX ? L"X" : L"", saw.flipY ? L"Y" : L"");
	}

	if (selectedObjectType_ != SelectedObjectType::None)
	{
		const int left =
			selectedObjectColumn_ * StageData::TileSize - cameraX_;
		const int top =
			selectedObjectRow_ * StageData::TileSize - cameraY_ +
			EditorToolbarHeight;
		const int right = left + StageData::TileSize;
		const int bottom = top + StageData::TileSize;
		const int selectionColor = GetColor(80, 240, 255);

		DrawBox(left, top, right, bottom, selectionColor, FALSE);
		DrawBox(left + 2, top + 2, right - 2, bottom - 2, selectionColor, FALSE);
		if (isDraggingObject_ && isHoveringStage_ &&
			(hoveredColumn_ != selectedObjectColumn_ ||
				hoveredRow_ != selectedObjectRow_))
		{
			const int targetLeft = hoveredColumn_ * StageData::TileSize - cameraX_;
			const int targetTop = hoveredRow_ * StageData::TileSize - cameraY_ +
				EditorToolbarHeight;
			DrawBox(targetLeft, targetTop, targetLeft + StageData::TileSize,
				targetTop + StageData::TileSize, selectionColor, FALSE);
		}
	}

	// マウスが指しているタイルを半透明の色と枠線で強調する。
	if (isHoveringStage_)
	{
		const int hoverLeft =
			hoveredColumn_ * StageData::TileSize - cameraX_;
		const int hoverTop =
			hoveredRow_ * StageData::TileSize - cameraY_ + EditorToolbarHeight;
		const int hoverRight = hoverLeft + StageData::TileSize;
		const int hoverBottom = hoverTop + StageData::TileSize;

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
		DrawBox(
			hoverLeft,
			hoverTop,
			hoverRight,
			hoverBottom,
			GetColor(255, 220, 60),
			TRUE
		);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		DrawBox(
			hoverLeft,
			hoverTop,
			hoverRight,
			hoverBottom,
			GetColor(255, 240, 80),
			FALSE
		);
	}

	// 指摘があるマスを地形より手前に赤枠で表示する。
	if (showValidation_)
	{
		for (const StageValidationIssue& issue : validationIssues_)
		{
			if (issue.column < 0 || issue.row < 0)
			{
				continue;
			}
			const int left = issue.column * StageData::TileSize - cameraX_;
			const int top = issue.row * StageData::TileSize - cameraY_ +
				EditorToolbarHeight;
			DrawBox(left, top, left + StageData::TileSize,
				top + StageData::TileSize, GetColor(255, 75, 75), FALSE);
		}
	}

	// ステージ描画を終えたら全画面描画へ戻し、固定UIを重ねる。
	SetDrawArea(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight);
	DrawBox(0, 0, EditorViewportWidth, EditorToolbarHeight,
		GetColor(5, 8, 14), TRUE);
	DrawBox(EditorViewportWidth, 0, GameConfig::ScreenWidth,
		GameConfig::ScreenHeight, GetColor(9, 14, 22), TRUE);
	DrawLine(EditorViewportWidth, 0, EditorViewportWidth,
		GameConfig::ScreenHeight, GetColor(80, 125, 150));

	UiFont::DrawString(16, 8, L"ステージエディター", accentColor);
	if (history_.IsDirty() || isStrokeDirty_)
	{
		UiFont::DrawString(190, 8, L"* 未保存", GetColor(255, 190, 80));
	}
	UiFont::DrawFormatString(16, 32, textColor,
		L"ステージ%d　サイズ%d x %d　カメラ%d, %d",
		stageData.stageNumber, StageData::TileColumns,
		stageData.tileRowCount, cameraX_, cameraY_);
	UiFont::DrawString(360, 8,
		moveMessageTimer_ > 0 ? L"移動できません" :
		previewMessageTimer_ > 0 ? L"SHIFT+T：開始位置が無効です" :
		L"T：テスト　SHIFT+T：カーソル位置から",
		(moveMessageTimer_ > 0 || previewMessageTimer_ > 0)
			? GetColor(255, 100, 100) : textColor);
	if (isHoveringStage_)
	{
		UiFont::DrawFormatString(760, 8, GetColor(255, 240, 80),
			L"タイル %d, %d", hoveredColumn_, hoveredRow_);
	}
	if (saveMessageTimer_ > 0)
	{
		UiFont::DrawString(760, 32,
			lastSaveSucceeded_
				? (validationIssues_.empty() ? L"保存しました" : L"保存済み / 要確認")
				: L"保存に失敗しました",
			lastSaveSucceeded_ ? GetColor(100, 255, 150)
				: GetColor(255, 100, 100));
	}

	UiFont::DrawString(SidebarFirstColumn, 60, L"地形", accentColor);
	DrawPaletteButton(SidebarFirstColumn, TerrainFirstRow,
		L"1  床", StageData::FloorTile);
	DrawPaletteButton(SidebarSecondColumn, TerrainFirstRow,
		L"2  壁", StageData::WallTile);
	DrawPaletteButton(SidebarFirstColumn, TerrainSecondRow,
		L"3  ブロック", StageData::BlockTile);
	DrawPaletteButton(SidebarSecondColumn, TerrainSecondRow,
		L"N  透明壁", StageData::InvisibleBlockTile);

	UiFont::DrawString(SidebarFirstColumn, 146, L"オブジェクト / ツール", accentColor);
	DrawPaletteButton(SidebarFirstColumn, ObjectFirstRow,
		L"4  プレイヤー", StageData::PlayerStartTile);
	DrawPaletteButton(SidebarSecondColumn, ObjectFirstRow,
		L"5  ゴール", StageData::GoalTile);
	DrawPaletteButton(SidebarFirstColumn, ObjectSecondRow,
		L"6  重力ブロック", StageData::GravityBlockTile);
	DrawPaletteButton(SidebarSecondColumn, ObjectSecondRow,
		L"Q  ノコギリ", SawTool);
	DrawPaletteButton(SidebarFirstColumn, ObjectThirdRow,
		L"F  穴", FallHoleTool);
	DrawPaletteButton(SidebarSecondColumn, ObjectThirdRow,
		L"C  カメラ", CameraAreaTool);
	DrawPaletteButton(SidebarFirstColumn, ObjectFourthRow,
		L"K  復活地点", StageData::CheckpointTile);
	DrawPaletteButton(SidebarSecondColumn, ObjectFourthRow,
		L"0  選択", SelectTool);

	UiFont::DrawString(SidebarFirstColumn, 298, L"敵", accentColor);
	DrawPaletteButton(SidebarFirstColumn, EnemyFirstRow,
		L"7  通常敵", StageData::NormalEnemyTile);
	DrawPaletteButton(SidebarSecondColumn, EnemyFirstRow,
		L"8  重量敵", StageData::HeavyEnemyTile);
	DrawPaletteButton(SidebarFirstColumn, EnemySecondRow,
		L"9  射撃敵", StageData::ShooterEnemyTile);
	DrawPaletteButton(SidebarSecondColumn, EnemySecondRow,
		L"U  上方射撃", StageData::UpShooterEnemyTile);
	DrawPaletteButton(SidebarFirstColumn, EnemyThirdRow,
		L"J  突進敵", StageData::ChargerEnemyTile);
	DrawPaletteButton(SidebarSecondColumn, EnemyThirdRow,
		L"L  迫撃敵", StageData::MortarEnemyTile);
	DrawPaletteButton(SidebarFirstColumn, EnemyFourthRow,
		L"O  中ボス", MidBossTool);
	DrawPaletteButton(SidebarSecondColumn, EnemyFourthRow,
		L"B  大ボス", BossTool);
	DrawPaletteButton(SidebarFirstColumn, ImageBrowserButtonTop,
		L"I  タイル画像", VisualBrushTool);
	DrawPaletteButton(SidebarSecondColumn, ImageBrowserButtonTop,
		L"H  往復ブロック", MovingGravityBlockTool);

	UiFont::DrawString(SidebarFirstColumn, 496, L"ショートカット", accentColor);
	UiFont::DrawString(SidebarFirstColumn, 520,
		L"T テスト　Shift+T カーソルから", textColor);
	UiFont::DrawString(SidebarFirstColumn, 542,
		L"Ctrl+S 保存　Ctrl+Z/Y 戻す/進む", textColor);
	UiFont::DrawString(SidebarFirstColumn, 564,
		L"WASD / 矢印 移動　Shift 高速", textColor);
	UiFont::DrawString(SidebarFirstColumn, 586,
		L"Shift+ドラッグ 矩形　V 検証", textColor);

	const int settingColor = GetColor(255, 230, 100);
	UiFont::DrawString(SidebarFirstColumn, 614, L"現在の設定", accentColor);
	if (selectedObjectType_ == SelectedObjectType::Enemy)
	{
		UiFont::DrawFormatString(SidebarFirstColumn, 638, settingColor,
			L"敵HP %d　Z -1 / X +1",
			editorStageData_.GetEnemyHitPointAt(
				selectedObjectColumn_, selectedObjectRow_));
		UiFont::DrawFormatString(SidebarFirstColumn, 660, settingColor,
			L"G 重力反転：%s",
			editorStageData_.IsEnemyAffectedByGravityReversalAt(
				selectedObjectColumn_, selectedObjectRow_)
				? L"受ける" : L"受けない");
		UiFont::DrawFormatString(SidebarFirstColumn, 682, settingColor,
			L"R 向き：%s",
			editorStageData_.IsEnemyFacingRightAt(
				selectedObjectColumn_, selectedObjectRow_)
				? L"右" : L"左");
	}
	else if (selectedObjectType_ == SelectedObjectType::GravityBlock)
	{
		const GravityBlockMotion motion =
			editorStageData_.GetGravityBlockMotionAt(
				selectedObjectColumn_, selectedObjectRow_);
		if (motion == GravityBlockMotion::Gravity)
		{
			UiFont::DrawString(SidebarFirstColumn, 638,
				L"通常の重力ブロック", settingColor);
		}
		else
		{
			UiFont::DrawFormatString(SidebarFirstColumn, 638, settingColor,
				L"往復方向：%s　R 切替",
				motion == GravityBlockMotion::Vertical ? L"上下" : L"左右");
			UiFont::DrawString(SidebarFirstColumn, 660,
				L"重力反転の影響を受けない", settingColor);
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::Saw)
	{
		const SawSpawnData* saw = editorStageData_.GetSawAt(
			selectedObjectColumn_, selectedObjectRow_);
		if (saw)
		{
			UiFont::DrawFormatString(SidebarFirstColumn, 630, settingColor,
				L"X 左右反転：%s", saw->flipX ? L"あり" : L"なし");
			UiFont::DrawFormatString(SidebarFirstColumn, 652, settingColor,
				L"Z 上下反転：%s", saw->flipY ? L"あり" : L"なし");
		}
	}
	else if (selectedObjectType_ == SelectedObjectType::MidBoss)
	{
		const MidBossSpawnData& spawn = stageData.midBossSpawn;
		UiFont::DrawFormatString(SidebarFirstColumn, 638, settingColor,
			L"ボスHP %d　Z -1 / X +1", spawn.hitPoint);
		UiFont::DrawFormatString(SidebarFirstColumn, 660, settingColor,
			L"グループ%d　重力：独立", spawn.cameraGroupId);
		UiFont::DrawFormatString(SidebarFirstColumn, 682, settingColor,
			L"R 向き：%s",
			spawn.facing == EnemySpawnFacing::Right ? L"右" : L"左");
	}
	else if (selectedObjectType_ == SelectedObjectType::Boss)
	{
		const BossSpawnData& spawn = stageData.bossSpawn;
		UiFont::DrawFormatString(SidebarFirstColumn, 638, settingColor,
			L"ボスHP %d　Z -1 / X +1", spawn.hitPoint);
		UiFont::DrawFormatString(SidebarFirstColumn, 660, settingColor,
			L"グループ%d　重力：独立", spawn.cameraGroupId);
		UiFont::DrawFormatString(SidebarFirstColumn, 682, settingColor,
			L"R 向き：%s",
			spawn.facing == EnemySpawnFacing::Right ? L"右" : L"左");
	}
	else if (selectedTile_ == CameraAreaTool)
	{
		UiFont::DrawFormatString(SidebarFirstColumn, 630, settingColor,
			L"範囲 %d/%d　クリック：選択",
			stageData.cameraAreaCount, StageData::MaxCameraAreas);
		UiFont::DrawString(SidebarFirstColumn, 652,
			L"Shift+ドラッグ：新規 / Ctrl：追加", settingColor);
		UiFont::DrawString(SidebarFirstColumn, 674,
			L"R+ドラッグ：変形 / M：追従 / 右：削除", settingColor);
	}
	else if (selectedTile_ == VisualBrushTool)
	{
		const wchar_t* role = selectedVisualRole_ == StageData::EmptyTile
			? L"背景" : selectedVisualRole_ == StageData::WallTile
			? L"壁" : selectedVisualRole_ == StageData::BlockTile
			? L"ブロック" : L"床";
		UiFont::DrawFormatString(SidebarFirstColumn, 630, settingColor,
			L"セル %d,%d　コマ数 %d",
			selectedSourceColumn_, selectedSourceRow_, selectedFrameCount_);
		UiFont::DrawFormatString(SidebarFirstColumn, 652, settingColor,
			L"種類 %s", role);
		UiFont::DrawFormatString(SidebarFirstColumn, 674, settingColor,
			L"X/Y反転：%s / %s",
			selectedVisualFlipX_ ? L"あり" : L"なし",
			selectedVisualFlipY_ ? L"あり" : L"なし");
	}
	else
	{
		UiFont::DrawString(SidebarFirstColumn, 630,
			L"ボタンかキーでツールを選択", textColor);
		UiFont::DrawString(SidebarFirstColumn, 652,
			L"選択中のツールは黄色い枠で表示", textColor);
	}

	if (showValidation_)
	{
		const int panelTop = GameConfig::ScreenHeight - 94;
		DrawBox(10, panelTop, 560, GameConfig::ScreenHeight - 10,
			GetColor(20, 26, 38), TRUE);
		UiFont::DrawFormatString(20, panelTop + 8, accentColor,
			L"検証：%d件　重力B:%d/%d　敵:%d/%d　穴:%d/%d　CP:%d/%d　TAB：次",
			static_cast<int>(validationIssues_.size()),
			stageData.gravityBlockSpawnCount, StageData::MaxGravityBlockSpawns,
			stageData.enemySpawnCount, StageData::MaxEnemySpawns,
			stageData.fallHoleCount, StageData::MaxFallHoles,
			stageData.checkpointCount, StageData::MaxCheckpoints);
		if (!validationIssues_.empty())
		{
			const StageValidationIssue& issue =
				validationIssues_[selectedIssueIndex_];
			UiFont::DrawFormatString(20, panelTop + 37, GetColor(255, 120, 120),
				L"%d/%d：%s", selectedIssueIndex_ + 1,
				static_cast<int>(validationIssues_.size()), issue.message);
			if (issue.column >= 0 && issue.row >= 0)
			{
				UiFont::DrawFormatString(20, panelTop + 61, textColor,
					L"タイル：%d, %d", issue.column, issue.row);
			}
		}
		else
		{
			UiFont::DrawString(20, panelTop + 37, L"問題はありません", GetColor(120, 255, 170));
		}
	}
	if (isImageBrowserOpen_)
	{
		DrawImageBrowser();
	}
}

// 選択中のタイルだけ枠と文字色を変え、現在のブラシを分かりやすくする。
void StageEditor::DrawPaletteButton(
	int left,
	int top,
	const wchar_t* text,
	char tile) const
{
	const bool isSelected = selectedTile_ == tile;
	const int borderColor = isSelected
		? GetColor(255, 240, 80)
		: GetColor(100, 130, 150);
	const int textColor = isSelected
		? GetColor(255, 240, 80)
		: GetColor(210, 220, 230);

	DrawBox(
		left,
		top,
		left + PaletteButtonWidth,
		top + PaletteButtonHeight,
		GetColor(25, 35, 48),
		TRUE
	);
	DrawBox(
		left,
		top,
		left + PaletteButtonWidth,
		top + PaletteButtonHeight,
		borderColor,
		FALSE
	);
	UiFont::DrawString(left + 14, top + 5, text, textColor);
}

void StageEditor::DrawObjectMarker(
	const Vector2& position,
	const wchar_t* text,
	int color) const
{
	const int left = static_cast<int>(position.x) - cameraX_;
	const int top =
		static_cast<int>(position.y) - cameraY_ + EditorToolbarHeight;
	const int right = left + StageData::TileSize;
	const int bottom = top + StageData::TileSize;

	if (right < 0 || left >= EditorViewportWidth ||
		bottom < EditorToolbarHeight || top >= GameConfig::ScreenHeight)
	{
		return;
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
	DrawBox(left, top, right, bottom, color, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBox(left, top, right, bottom, GetColor(255, 255, 255), FALSE);
	UiFont::DrawString(left + 14, top + 10, text, GetColor(20, 25, 30));
}

// 画面右端・下端より先へカメラが移動しないようにする。
void StageEditor::ClampCamera()
{
	int maxCameraX = worldWidth_ - EditorViewportWidth;
	int maxCameraY =
		worldHeight_ - (GameConfig::ScreenHeight - EditorToolbarHeight);

	if (maxCameraX < 0)
	{
		maxCameraX = 0;
	}

	if (maxCameraY < 0)
	{
		maxCameraY = 0;
	}

	cameraX_ = ClampInt(cameraX_, 0, maxCameraX);
	cameraY_ = ClampInt(cameraY_, 0, maxCameraY);
}
