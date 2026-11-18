#pragma once

#include "Player.h"
#include "EnemyManager.h"
#include "InputManager.h"
#include "GravitySystem.h"
#include "BulletManager.h"
#include "EnemyBulletManager.h"
#include "Laser.h"
#include "StageManager.h"
#include "Goal.h"
#include "Boss.h"
#include "EffectManager.h"
#include "ScoreManager.h"
#include "GameState.h"
#include "StageRenderer.h"
#include "GravityBlockManager.h"
#include "StageEditor.h"
#include "CameraTransition.h"
#include "SawHazard.h"
#include "GravityRecoveryDevice.h"
#include "AudioManager.h"

class Application
{
private:

	bool Initialize();
	void Update();
	void Draw();
	void DrawTitleBackground() const;
	void DrawTitleScreen();
	void DrawTitleControls();
	void DrawTitleStageSelect();
	void DrawGameClearScreen();
	void DrawGameOverScreen();
	void MoveMenuSelection(int& selectedItem, int itemCount);
	void DrawMenuItem(int itemY, const wchar_t* text, bool isSelected) const;
	void DrawCenteredUiText(
		int y,
		const wchar_t* text,
		unsigned int color,
		int fontSize = 16) const;
	void DrawScreenPanel(int left, int top, int right, int bottom, int borderColor) const;
	void DrawGameWorld();
	void DrawGameUi();
	void DrawDebugGrid();
	void DrawPauseOverlay();
	void Finalize();
	void CheckStageClear();
	void StartCurrentStage();
	void StartEditorPreview(bool fromCursor);
	void ReturnToStageEditor();
	void GoToNextStage();
	bool HandleDebugStageJump();
	void CheckPlayerDamage();
	void CheckFallLoop();
	void CheckCheckpointActivation();
	void CheckBossActivation();
	void PlayCheckpointEffect(int checkpointIndex);
	void RespawnAtCheckpoint();
	void DrawCheckpoints() const;
	void StartPlayerDeath();
	void StartScreenShake(int duration, int power);
	void StartDirectionalScreenShake(int duration, int power, int directionX);
	void UpdateScreenShake();
	void UpdateCamera();
	void GetScreenShakeOffset(int& offsetX, int& offsetY) const;
	bool IsPlayerCollidingWithBoss() const;

	bool isRunning_ = true;
	int gameScreenHandle_ = -1;
	int worldScreenHandle_ = -1;
	int hpGaugeImageHandle_ = -1;
	int laserGaugeImageHandle_ = -1;
	int checkpointImageHandle_ = -1;
	int titleStarImageHandle_ = -1;
	int menuButtonImageHandle_ = -1;
	// ワールド画像の左上から、画面へ切り出す位置。
	int cameraX_ = 0;
	int cameraY_ = 0;
	CameraTransition cameraTransition_;
	bool isEditorPreviewActive_ = false;

	// タイトル画面で選択中のメニュー番号。
	int selectedTitleMenuItem_ = 0;

	// タイトル画面で操作説明を開いているかどうか。
	bool isTitleControlsOpen_ = false;

	// タイトル画面でステージセレクトを開いているかどうか。
	bool isTitleStageSelectOpen_ = false;

	// ステージセレクトを、ゲーム開始ではなくエディター用に開いているかどうか。
	bool isSelectingStageForEditor_ = false;

	// ステージセレクトで選択中のステージ番号。
	int selectedStageSelectItem_ = 0;

	// ポーズ画面で選択中のメニュー番号。
	int selectedPauseMenuItem_ = 0;

	// 死亡画面で選択中のメニュー番号。
	int selectedDeathMenuItem_ = 0;

	// ポーズ画面で操作説明を開いているかどうか。
	bool isPauseControlsOpen_ = false;

	// メニューカーソルを点滅させるためのカウンター。
	int menuCursorBlinkTimer_ = 0;

	// ステージ編集用のグリッドを表示するかどうか。
	bool isDebugGridVisible_ = false;

	// ステージ開始表示中か確認する。
	bool IsStageStarting() const;

	// ステージクリア演出中か確認する。
	bool IsStageClearWaiting() const;

	// タイトルメニューの項目数。
	static constexpr int TitleMenuItemCount = 5;

	// ポーズメニューの項目数。
	static constexpr int PauseMenuItemCount = 4;

	// 死亡メニューの項目数。
	static constexpr int DeathMenuItemCount = 3;

	// ステージセレクトの項目数。
	static constexpr int StageSelectItemCount = 5;

	// ボス撃破後、ステージクリアへ進むまで待つ時間。
	static constexpr int BossClearWaitFrames = 300;

	// ステージクリアから次のステージに進むまでの待ち時間。
	static constexpr int StageClearWaitFrames = 90;

	// ゲームクリア画面でスコアを0から加算表示する時間。
	static constexpr int GameClearScoreCountFrames = 90;

	// ステージ開始時の表示時間。
	static constexpr int StageStartDisplayFrames = 90;

	// ブロック着地時に画面を揺らす時間。
	static constexpr int ScreenShakeFrames = 10;

	// ブロック着地時の揺れ幅。
	static constexpr int ScreenShakePower = 4;

	// ボス撃破時に画面を揺らす時間。
	static constexpr int BossDefeatScreenShakeFrames = 20;

	// ボス撃破時の揺れ幅。
	static constexpr int BossDefeatScreenShakePower = 8;

	// プレイヤーダメージ時に画面を揺らす時間。
	static constexpr int PlayerDamageScreenShakeFrames = 8;

	// プレイヤーダメージ時の揺れ幅。
	static constexpr int PlayerDamageScreenShakePower = 3;

	// レーザー発射時に横方向へ揺らす時間。
	static constexpr int LaserFireScreenShakeFrames = 8;

	// レーザー発射時の横揺れ幅。
	static constexpr int LaserFireScreenShakePower = 4;

	// プレイヤー死亡エフェクトを見せる時間。
	static constexpr int PlayerDeathWaitFrames = 60;

	// プレイヤー死亡時に画面を揺らす時間。
	static constexpr int PlayerDeathScreenShakeFrames = 20;

	// プレイヤー死亡時の揺れ幅。
	static constexpr int PlayerDeathScreenShakePower = 8;

	// ボス撃破後の待機残り時間。
	int bossClearWaitTimer_ = 0;

	// クリア後の待機残り時間。
	int stageClearWaitTimer_ = 0;

	// ゲームクリア画面に表示している途中のスコア。
	int displayedGameClearScore_ = 0;

	// ステージ開始表示の残り時間。
	int stageStartTimer_ = 0;

	// プレイヤー死亡後、ゲームオーバー画面へ移るまでの残り時間。
	int playerDeathWaitTimer_ = 0;
	int activeCheckpointIndex_ = -1;
	int touchingCheckpointIndex_ = -1;
	Vector2 checkpointRespawnPosition_{};
	GravityDirection checkpointGravityDirection_ = GravityDirection::Down;
	bool hasCheckpointRespawnPosition_ = false;

	// ゲームオーバーから再挑戦した直後のステージ開始表示かどうか。
	bool isRetryStart_ = false;

	// 画面揺れの残り時間。
	int screenShakeTimer_ = 0;

	// 画面揺れの現在の強さ。
	int screenShakePower_ = 0;

	// 0なら通常揺れ、-1または1なら横方向固定の揺れ。
	int screenShakeDirectionX_ = 0;

	InputManager inputManager_;
	Player player_;
	EnemyManager enemyManager_;
	GravitySystem gravitySystem_;
	BulletManager bulletManager_;
	EnemyBulletManager enemyBulletManager_;
	Laser laser_;
	StageManager stageManager_;
	StageRenderer stageRenderer_;
	GravityBlockManager gravityBlockManager_;
	GravityRecoveryDevice gravityRecoveryDevice_;
	AudioManager audioManager_;
	SawHazard sawHazard_;
	StageEditor stageEditor_;
	Goal goal_;
	Boss boss_;
	EffectManager effectManager_;
	ScoreManager scoreManager_;

	GameState gameState_ = GameState::Title;

public:
	int Run();
};
