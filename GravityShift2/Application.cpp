#include "Application.h"
#include "CameraConfig.h"
#include <cwchar>
#include "GameConfig.h"
#include "UiFont.h"
#include "DxLib.h"

namespace
{
	// プレイヤーより先の上方向を見やすくするため、カメラを上へずらす量。
	constexpr int CameraVerticalOffset = 80;
	// 穴の通過中もプレイヤーを画面内に残すための上下余白。
	constexpr int FallHoleCameraVerticalMargin = 90;
	// valueをminValue以上、maxValue以下の範囲に収める。
	// C++17のstd::clampを使わず、現在のプロジェクト設定でも動くようにする。
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
}

// アプリケーション全体の起動準備を行う。
// DXライブラリの初期化、描画先設定、ゲーム内オブジェクトの初期化をまとめる。
bool Application::Initialize()
{
	// ウィンドウモードで起動する。
	ChangeWindowMode(TRUE);

	SetGraphMode(
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		GameConfig::ColorBitDepth
	);

	if(DxLib_Init() == -1)
	{
		return false;
	}
	if (!UiFont::Load())
	{
		return false;
	}

	SetDrawScreen(DX_SCREEN_BACK);

	gameScreenHandle_ = MakeScreen(
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		TRUE
	);

	if (gameScreenHandle_ == -1)
	{
		return false;
	}

	worldScreenHandle_ = MakeScreen(
		CameraConfig::ViewWidth,
		CameraConfig::ViewHeight,
		TRUE
	);

	if (worldScreenHandle_ == -1)
	{
		return false;
	}

	hpGaugeImageHandle_ = LoadGraph(L"Assets/Images/UI/player-hp-gauge.png");
	if (hpGaugeImageHandle_ == -1)
	{
		hpGaugeImageHandle_ = LoadGraph(L"../../Assets/Images/UI/player-hp-gauge.png");
	}
	laserGaugeImageHandle_ = LoadGraph(L"Assets/Images/UI/laser-gauge.png");
	if (laserGaugeImageHandle_ == -1)
	{
		laserGaugeImageHandle_ = LoadGraph(L"../../Assets/Images/UI/laser-gauge.png");
	}
	checkpointImageHandle_ =
		LoadGraph(L"Assets/Images/Objects/checkpoint.png");
	if (checkpointImageHandle_ == -1)
	{
		checkpointImageHandle_ =
			LoadGraph(L"../../Assets/Images/Objects/checkpoint.png");
	}
	titleStarImageHandle_ =
		LoadGraph(L"Assets/Images/UI/title-star-sheet.png");
	if (titleStarImageHandle_ == -1)
	{
		titleStarImageHandle_ =
			LoadGraph(L"../../Assets/Images/UI/title-star-sheet.png");
	}
	menuButtonImageHandle_ =
		LoadGraph(L"Assets/Images/UI/menu-button.png");
	if (menuButtonImageHandle_ == -1)
	{
		menuButtonImageHandle_ =
			LoadGraph(L"../../Assets/Images/UI/menu-button.png");
	}
	goal_.Load();

	// ステージ管理を初期化し、ステージ1から開始する。
	stageManager_.Initialize();
	// スコアを初期化する。
	scoreManager_.Reset();
	player_.LoadSprites();

	// 敵用スプライトを読み込む。
    // 画像がまだ無い場合でも、EnemyManager側で矩形描画へ戻す。
	if (!enemyManager_.Load())
	{
		return false;
	}
	if (!boss_.Load())
	{
		return false;
	}
	if (!gravityRecoveryDevice_.Load())
	{
		return false;
	}
	if (!audioManager_.Load())
	{
		return false;
	}

	// 重力ブロック用スプライトを読み込む。
	if (!gravityBlockManager_.Load())
	{
		return false;
	}

	sawHazard_.Load();

	// ステージ用タイルセット画像を読み込む。
	if (!stageRenderer_.Load())
	{
		return false;
	}

	// エフェクト画像を読み込む。
	if (!effectManager_.Load())
	{
		return false;
	}

	// 弾用スプライトを読み込む。
	if (!bulletManager_.Load())
	{
		return false;
	}

	laser_.SetImageHandle(bulletManager_.GetImageHandle());

	// 敵弾をカメラ座標に追従させて描画するため、画像として準備する。
	if (!enemyBulletManager_.Load())
	{
		return false;
	}

	StartCurrentStage();

	gameState_ = GameState::Title;
	audioManager_.PlayTitleBgm();

	return true;
}

// 1フレーム分のゲーム状態を更新する。
// 入力更新 → システム操作 → プレイヤー更新 → 弾更新、の順に処理する。
void Application::Update()
{
	// ウィンドウが閉じられた場合はゲームループを終了する。
	if (ProcessMessage() != 0)
	{
		isRunning_ = false;
		return;
	}

	inputManager_.Update();
	if (gameState_ == GameState::StageEditor || gameState_ == GameState::Playing)
	{
		stageRenderer_.UpdateAnimation();
	}
	UpdateScreenShake();
	++menuCursorBlinkTimer_;

	// エディター中はゲーム本編を更新せず、編集用カメラだけを動かす。
	if (gameState_ == GameState::StageEditor)
	{
		const bool browserWasOpen = stageEditor_.IsImageBrowserOpen();
		stageEditor_.Update(inputManager_);
		if (!browserWasOpen && !stageEditor_.IsImageBrowserOpen() &&
			inputManager_.IsTriggered(KEY_INPUT_T))
		{
			StartEditorPreview(
				inputManager_.IsPressed(KEY_INPUT_LSHIFT) ||
				inputManager_.IsPressed(KEY_INPUT_RSHIFT));
			return;
		}

		if (!browserWasOpen && !stageEditor_.IsImageBrowserOpen() &&
			inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			selectedTitleMenuItem_ = 0;
			isSelectingStageForEditor_ = false;
			audioManager_.PlayTitleBgm();
			gameState_ = GameState::Title;
		}

		return;
	}

	// テストプレイ中のESCはゲーム終了ではなく、編集画面へ戻す。
	if (isEditorPreviewActive_ && inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
	{
		ReturnToStageEditor();
		return;
	}

	if (!isEditorPreviewActive_ && HandleDebugStageJump())
	{
		return;
	}

	if (inputManager_.IsTriggered(KEY_INPUT_9))
	{
		isDebugGridVisible_ = !isDebugGridVisible_;
	}

	if (gameState_ == GameState::Title)
	{
		if (isTitleControlsOpen_)
		{
			if (inputManager_.IsCancelTriggered() ||
				inputManager_.IsConfirmTriggered())
			{
				if (inputManager_.IsConfirmTriggered())
				{
					audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
				}
				isTitleControlsOpen_ = false;
			}

			return;
		}

		if (isTitleStageSelectOpen_)
		{
			MoveMenuSelection(selectedStageSelectItem_, StageSelectItemCount);

			if (inputManager_.IsConfirmTriggered())
			{
				audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
				const int selectedStageNumber = selectedStageSelectItem_ + 1;
				stageManager_.StartStage(selectedStageNumber);

				if (isSelectingStageForEditor_)
				{
					stageEditor_.Initialize(stageManager_.GetCurrentStage(),
						&stageRenderer_);
					audioManager_.StopBgm();
					gameState_ = GameState::StageEditor;
				}
				else
				{
					scoreManager_.Reset();
					StartCurrentStage();
					gameState_ = GameState::Playing;
				}

				isTitleStageSelectOpen_ = false;
			}

			if (inputManager_.IsCancelTriggered())
			{
				isTitleStageSelectOpen_ = false;
				isSelectingStageForEditor_ = false;
			}

			return;
		}

		MoveMenuSelection(selectedTitleMenuItem_, TitleMenuItemCount);

		if (inputManager_.IsConfirmTriggered())
		{
			audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
			if (selectedTitleMenuItem_ == 0)
			{
				stageManager_.Initialize();
				scoreManager_.Reset();
				StartCurrentStage();

				gameState_ = GameState::Playing;
			}
			else if (selectedTitleMenuItem_ == 1)
			{
				selectedStageSelectItem_ = 0;
				isSelectingStageForEditor_ = false;
				isTitleStageSelectOpen_ = true;
			}
			else if (selectedTitleMenuItem_ == 2)
			{
				isTitleControlsOpen_ = true;
			}
			else if (selectedTitleMenuItem_ == 3)
			{
				// 編集対象を選んでからエディターを開く。
				selectedStageSelectItem_ = 0;
				isSelectingStageForEditor_ = true;
				isTitleStageSelectOpen_ = true;
			}
			else
			{
				isRunning_ = false;
			}
		}

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}

	if (gameState_ == GameState::GameClear)
	{
		const int finalScore = scoreManager_.GetScore();
		if (displayedGameClearScore_ < finalScore)
		{
			const int scoreStep =
				(finalScore + GameClearScoreCountFrames - 1) /
				GameClearScoreCountFrames;
			displayedGameClearScore_ = ClampInt(
				displayedGameClearScore_ + scoreStep,
				0,
				finalScore
			);
		}

		if (inputManager_.IsConfirmTriggered())
		{
			audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
			stageManager_.Initialize();
			scoreManager_.Reset();
			StartCurrentStage();
			selectedTitleMenuItem_ = 0;
			isTitleControlsOpen_ = false;
			isTitleStageSelectOpen_ = false;
			isSelectingStageForEditor_ = false;

			audioManager_.PlayTitleBgm();
			gameState_ = GameState::Title;
		}

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}


	if (gameState_ == GameState::Pause)
	{
		if (isPauseControlsOpen_)
		{
			if (inputManager_.IsCancelTriggered() ||
				inputManager_.IsConfirmTriggered())
			{
				if (inputManager_.IsConfirmTriggered())
				{
					audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
				}
				isPauseControlsOpen_ = false;
			}

			return;
		}

		// STARTで開いたポーズはSTART、B、BACKのどれでも素早く再開できる。
		if (inputManager_.IsPauseTriggered() ||
			inputManager_.IsGamepadTriggered(GamepadButton::B) ||
			inputManager_.IsGamepadTriggered(GamepadButton::Back))
		{
			gameState_ = GameState::Playing;
			return;
		}

		MoveMenuSelection(selectedPauseMenuItem_, PauseMenuItemCount);

		if (inputManager_.IsConfirmTriggered())
		{
			audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
			if (selectedPauseMenuItem_ == 0)
			{
				gameState_ = GameState::Playing;
			}
			else if (selectedPauseMenuItem_ == 1)
			{
				isPauseControlsOpen_ = true;
			}
			else if (selectedPauseMenuItem_ == 2)
			{
				if (isEditorPreviewActive_)
				{
					ReturnToStageEditor();
					return;
				}
				// タイトルへ戻るときは、ゲーム中に残っている攻撃状態を消しておく。
				bulletManager_.Clear();
				laser_.Reset();
				enemyBulletManager_.Clear();
				selectedTitleMenuItem_ = 0;
				isTitleControlsOpen_ = false;
				isTitleStageSelectOpen_ = false;
				isSelectingStageForEditor_ = false;
				isPauseControlsOpen_ = false;
				audioManager_.PlayTitleBgm();
				gameState_ = GameState::Title;
			}
			else
			{
				isRunning_ = false;
			}
		}

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}

	if (gameState_ == GameState::GameOver)
	{
		MoveMenuSelection(selectedDeathMenuItem_, DeathMenuItemCount);

		if (inputManager_.IsConfirmTriggered())
		{
			audioManager_.PlaySoundEffect(SoundEffect::MenuConfirm);
			if (selectedDeathMenuItem_ == 0)
			{
				RespawnAtCheckpoint();
				audioManager_.PlaySoundEffect(SoundEffect::Restart);
				gameState_ = GameState::Playing;
			}
			else if (selectedDeathMenuItem_ == 1)
			{
				StartCurrentStage();
				audioManager_.PlaySoundEffect(SoundEffect::Restart);
				isRetryStart_ = true;
				gameState_ = GameState::Playing;
			}
			else if (isEditorPreviewActive_)
			{
				ReturnToStageEditor();
			}
			else
			{
				bulletManager_.Clear();
				laser_.Reset();
				enemyBulletManager_.Clear();
				effectManager_.Clear();
				selectedTitleMenuItem_ = 0;
				isTitleControlsOpen_ = false;
				isTitleStageSelectOpen_ = false;
				isSelectingStageForEditor_ = false;
				audioManager_.PlayTitleBgm();
				gameState_ = GameState::Title;
			}
		}

		return;
	}

	if (playerDeathWaitTimer_ > 0)
	{
		player_.AdvanceDeathAnimation();
		effectManager_.Update();
		scoreManager_.Update();
		--playerDeathWaitTimer_;

		if (playerDeathWaitTimer_ <= 0)
		{
			selectedDeathMenuItem_ = 0;
			gameState_ = GameState::GameOver;
		}

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}

	if (IsStageClearWaiting())
	{
		effectManager_.Update();
		scoreManager_.Update();

		if (bossClearWaitTimer_ > 0)
		{
			--bossClearWaitTimer_;

			if (bossClearWaitTimer_ <= 0)
			{
				GoToNextStage();
			}
		}
		else if (stageClearWaitTimer_ > 0)
		{
			--stageClearWaitTimer_;

			if (stageClearWaitTimer_ <= 0)
			{
				GoToNextStage();
			}
		}

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}
	

	if (IsStageStarting())
	{
		--stageStartTimer_;

		if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
		{
			isRunning_ = false;
		}

		return;
	}

	// キーボードのPまたはコントローラーのSTARTでゲームを一時停止する。
	if (inputManager_.IsPauseTriggered())
	{
		selectedPauseMenuItem_ = 0;
		isPauseControlsOpen_ = false;
		gameState_ = GameState::Pause;
		return;
	}

	// キーボードのGまたはコントローラーのYを押した瞬間だけ重力を反転する。
	// 反転時は、プレイヤーが以前の床・天井に接地中だった状態を解除する。
	if (inputManager_.IsGravityTriggered())
	{
		effectManager_.PlayGravityReverseEffect(
			player_.GetPosition(), player_.GetWidth(), player_.GetHeight(),
			gravitySystem_.GetDirection() == GravityDirection::Up);
		gravitySystem_.Reverse();
		audioManager_.PlaySoundEffect(SoundEffect::GravityReverse);
		player_.OnGravityReversed();
		enemyManager_.OnGravityReversed();
		boss_.OnGravityReversed();
		gravityBlockManager_.OnGravityReversed(
			static_cast<float>(cameraX_),
			static_cast<float>(cameraY_),
			static_cast<float>(CameraConfig::ViewWidth),
			static_cast<float>(CameraConfig::ViewHeight));
	}

	// キーボードのXまたはコントローラーのRBをレーザー入力として扱う。
	const bool isLaserFireTriggered =
		inputManager_.IsLaserTriggered();

	const StageData& currentStage =
		stageManager_.GetCurrentStage();

	// 重力反転ブロックを先に更新し、このフレームのブロック位置を確定する。
	sawHazard_.Update();
	gravityBlockManager_.Update(gravitySystem_, currentStage);
	if (gravityBlockManager_.PlayLandingEffects(effectManager_))
	{
		StartScreenShake(ScreenShakeFrames, ScreenShakePower);
	}

	// 落下中の重力ブロックがプレイヤーに当たったら、接地補正の前にダメージを与える。
	float damageSourceCenterX =
		player_.GetPosition().x + player_.GetWidth() * 0.5f;

	if (gravityBlockManager_.CheckPlayerCollision(player_, damageSourceCenterX))
	{
		if (player_.TakeDamage(damageSourceCenterX))
		{
			StartScreenShake(PlayerDamageScreenShakeFrames, PlayerDamageScreenShakePower);

			if (player_.IsDead())
			{
				StartPlayerDeath();
				return;
			}
			audioManager_.PlaySoundEffect(SoundEffect::PlayerDamage);
		}
	}

	// プレイヤーを更新して、床・天井との当たり判定まで行う。
	player_.Update(inputManager_, gravitySystem_, currentStage);
	if (player_.WasJumpedThisFrame())
	{
		audioManager_.PlaySoundEffect(SoundEffect::PlayerJump);
	}
	if (player_.WasDashStartedThisFrame())
	{
		audioManager_.PlaySoundEffect(SoundEffect::Dash);
	}

	// 穴から画面外へ抜けたら、反対側の穴から出現させる。
	CheckFallLoop();

	// プレイヤーがブロックへ横からぶつかっていたら、先に左右へ押し戻す。
	if (gravityBlockManager_.ResolvePlayerHorizontalCollision(player_))
	{
		// ブロックに押された先がステージ壁だった場合、壁の外へ戻す。
		player_.ResolveStageHorizontalCollision(currentStage);
	}

	// プレイヤーがブロックに乗っていたら、床・天井と同じように接地させる。
	gravityBlockManager_.ResolvePlayerVerticalCollision(player_, gravitySystem_);
	CheckCheckpointActivation();

	// プレイヤー位置の確定後にカメラを更新し、敵の待機範囲にも同じ位置を使う。
	UpdateCamera();
	CheckBossActivation();

	// 敵もプレイヤーと同じ重力方向で更新する。
	const int hitPointBeforeHealing = player_.GetHitPoint();
	enemyManager_.Update(
		gravitySystem_,
		currentStage,
		player_,
		enemyBulletManager_,
		cameraX_,
		cameraY_);

	// ボスが出現している場合だけ更新する。
	boss_.Update(gravitySystem_, currentStage, player_);
	gravityRecoveryDevice_.Update(player_, boss_.IsActive());
	if (player_.GetHitPoint() > hitPointBeforeHealing)
	{
		audioManager_.PlaySoundEffect(SoundEffect::Heal);
	}
	if (boss_.ConsumeGrenadeSoundRequest())
	{
		audioManager_.PlaySoundEffect(SoundEffect::BossGrenade);
	}
	if (boss_.ConsumeAttackWarningSoundRequest())
	{
		audioManager_.PlaySoundEffect(SoundEffect::BossCooldown);
	}

	const float laserDirection =
		static_cast<float>(player_.GetFacingDirection());

	laser_.Update(
		isLaserFireTriggered,
		player_.GetLaserStartPosition(),
		laserDirection
	);

	if (laser_.WasFiredThisFrame())
	{
		player_.OnLaserFire();
		audioManager_.PlaySoundEffect(SoundEffect::Laser);

		const int recoilDirectionX =
			player_.GetFacingDirection() == FacingDirection::Right ? -1 : 1;
		StartDirectionalScreenShake(
			LaserFireScreenShakeFrames,
			LaserFireScreenShakePower,
			recoilDirectionX
		);
	}

	// 入力状態ではなく、Laserクラスが決定した実際の状態を使う。
	player_.SetLaserActive(laser_.IsActive());

	// ZまたはBだけなら左右へ射撃する。
	// 上下入力と同時なら、現在の重力方向を基準に頭側・足元側へ撃ち分ける。
	if (inputManager_.IsShootTriggered() && !laser_.IsActive())
	{
		if (inputManager_.IsPressed(KEY_INPUT_UP))
		{
			const float headDirectionY =
				gravitySystem_.GetDirection() == GravityDirection::Down
				? -1.0f
				: 1.0f;
			bulletManager_.Shoot(
				player_.GetHeadShootPosition(),
				{ 0.0f, headDirectionY });
		}
		else if (inputManager_.IsPressed(KEY_INPUT_DOWN))
		{
			const float footDirectionY =
				gravitySystem_.GetDirection() == GravityDirection::Down
				? 1.0f
				: -1.0f;
			bulletManager_.Shoot(
				player_.GetFootShootPosition(),
				{ 0.0f, footDirectionY });
		}
		else
		{
			bulletManager_.Shoot(
				player_.GetShootPosition(),
				{ static_cast<float>(player_.GetFacingDirection()), 0.0f });
		}

		player_.OnShoot();
		audioManager_.PlaySoundEffect(SoundEffect::PlayerShot);
	}

	// 通常弾を移動し、地形や画面外に当たったものは無効化する。
	bulletManager_.Update(currentStage, gravityBlockManager_);

	// 敵が発射した弾を移動し、画面外に出たものは無効化する。
	enemyBulletManager_.Update(currentStage, gravityBlockManager_);

	effectManager_.Update();

	// スコアポップアップを更新する。
	scoreManager_.Update();

	// 敵やボス本体に触れていたら、プレイヤーにダメージを与える。
	CheckPlayerDamage();
	Vector2 bossExplosionEffectPosition{};
	while (boss_.ConsumeExplosionEffect(bossExplosionEffectPosition))
	{
		effectManager_.PlayEnemyDestroyEffect(bossExplosionEffectPosition);
	}

	// 落下中の重力ブロックが敵に当たったら、敵を押しつぶして倒す。
	gravityBlockManager_.CheckEnemyCollisions(
		enemyManager_,
		effectManager_,
		scoreManager_
	);

	// 全ての敵と、弾・レーザーの当たり判定をまとめて行う。
	enemyManager_.CheckWeaponCollisions(
		bulletManager_,
		laser_,
		effectManager_,
		scoreManager_
	);
	const bool enemyDeathSoundRequested =
		enemyManager_.ConsumeDeathSoundRequest();
	const bool enemyDamageSoundRequested =
		enemyManager_.ConsumeDamageSoundRequest();
	if (enemyDeathSoundRequested)
	{
		audioManager_.PlaySoundEffect(SoundEffect::EnemyDeath);
	}
	else if (enemyDamageSoundRequested)
	{
		audioManager_.PlaySoundEffect(SoundEffect::EnemyDamage);
	}


	const bool wasBossActive = boss_.IsActive();
	const bool wasBossDefeated = boss_.IsDefeated();
	const int bossHitPointBeforeDamage = boss_.GetHitPoint();

	// 発射中の弾とボスの当たり判定を行う。
	bulletManager_.CheckCollision(boss_);
	// 発射中のレーザーとボスの当たり判定を行う。
	if (laser_.CheckCollision(boss_))
	{
		laser_.OnDamageDealt();
	}

	const bool isBossDefeatedNow =
		wasBossActive &&
		!wasBossDefeated &&
		boss_.IsDefeated();

	if (isBossDefeatedNow)
	{
		audioManager_.StopBgm();
		audioManager_.PlaySoundEffect(SoundEffect::BossDeath);
		gravityRecoveryDevice_.Deactivate();
		const Vector2 bossDestroyEffectPosition
		{
			boss_.GetPosition().x + boss_.GetWidth() * 0.5f - 48.0f,
			boss_.GetPosition().y + boss_.GetHeight() * 0.5f - 48.0f
		};

		effectManager_.PlayBossDestroyEffect(bossDestroyEffectPosition);
		StartScreenShake(BossDefeatScreenShakeFrames, BossDefeatScreenShakePower);

		const int bossScore = 1000;

		const Vector2 bossScorePopupPosition
		{
			boss_.GetPosition().x + boss_.GetWidth() * 0.5f - 50.0f,
			boss_.GetPosition().y
		};

		// ボス撃破時は大きめの文字でスコアを表示する。
		scoreManager_.AddScore(
			bossScore,
			bossScorePopupPosition,
			36,
			GetColor(255, 220, 80)
		);

		bossClearWaitTimer_ = BossClearWaitFrames;
	}
	else if (boss_.GetHitPoint() < bossHitPointBeforeDamage)
	{
		audioManager_.PlaySoundEffect(SoundEffect::EnemyDamage);
	}

	// 通常プレイ中だけ、ステージクリア条件を確認する。
	CheckStageClear();

	if (!isEditorPreviewActive_ && inputManager_.IsTriggered(KEY_INPUT_N))
	{
		if(!stageManager_.IsGameCleared())
		{
			GoToNextStage();
		}
	}

	if (inputManager_.IsTriggered(KEY_INPUT_ESCAPE))
	{
		isRunning_ = false;
	}
}

// 同じX位置にある床穴と天井穴をつなぎ、抜けた側と反対の穴から出す。
void Application::CheckFallLoop()
{
	const StageData& currentStage =
		stageManager_.GetCurrentStage();

	if (currentStage.fallHoleCount <= 0)
	{
		return;
	}

	const float playerTop = player_.GetPosition().y;
	const float playerBottom = playerTop + player_.GetHeight();
	const float playerCenterX = player_.GetPosition().x + player_.GetWidth() * 0.5f;
	const float supportHalfWidth = player_.GetWidth() * 0.33f;
	float pairedExitY = 0.0f;
	if (currentStage.TryGetPairedFallHoleExitY(
		gravitySystem_.GetDirection() == GravityDirection::Down,
		playerCenterX - supportHalfWidth,
		playerCenterX + supportHalfWidth,
		playerTop,
		playerBottom,
		player_.GetHeight(),
		pairedExitY))
	{
		player_.SetPositionY(pairedExitY);
		return;
	}

	// 2項目形式の旧穴データは、従来どおりステージ外周でループする。
	bool opensFloor = false;
	bool opensCeiling = false;
	for (int i = 0; i < currentStage.fallHoleCount; ++i)
	{
		const FallHole& hole = currentStage.fallHoles[i];
		if (playerCenterX >= static_cast<float>(hole.left) &&
			playerCenterX < static_cast<float>(hole.right))
		{
			opensFloor = opensFloor || hole.OpensFloor(currentStage.GetPixelHeight());
			opensCeiling = opensCeiling || hole.OpensCeiling();
		}
	}

	// 下の穴へ落ちきったら、上の穴から出す。
	if (opensFloor && playerTop >= static_cast<float>(
		currentStage.GetPixelHeight() + StageData::FallLoopTravelMargin))
	{
		player_.SetPositionY(-player_.GetHeight());
		return;
	}

	// 上の穴へ抜けきったら、下の穴から出す。
	if (opensCeiling && playerBottom <=
		-static_cast<float>(StageData::FallLoopTravelMargin))
	{
		player_.SetPositionY(static_cast<float>(currentStage.GetPixelHeight()));
	}
}
// プレイヤーが敵やボスに触れていたら、HPを減らす。
void Application::CheckPlayerDamage()
{
	float damageSourceCenterX =
		player_.GetPosition().x + player_.GetWidth() * 0.5f;

	bool isPlayerHit = false;

	if (sawHazard_.CheckPlayerCollision(
		stageManager_.GetCurrentStage(), player_, damageSourceCenterX))
	{
		isPlayerHit = true;
	}
	else if (enemyManager_.CheckPlayerCollision(player_, damageSourceCenterX))
	{
		isPlayerHit = true;
	}
	else if (IsPlayerCollidingWithBoss())
	{
		damageSourceCenterX =
			boss_.GetPosition().x + boss_.GetWidth() * 0.5f;
		isPlayerHit = true;
	}
	else if (boss_.CheckProjectileCollision(player_, damageSourceCenterX))
	{
		isPlayerHit = true;
	}
	else if (enemyBulletManager_.CheckPlayerCollision(player_, damageSourceCenterX))
	{
		isPlayerHit = true;
	}

	if (!isPlayerHit)
	{
		return;
	}

	if (!player_.TakeDamage(damageSourceCenterX))
	{
		return;
	}

	StartScreenShake(PlayerDamageScreenShakeFrames, PlayerDamageScreenShakePower);

	if (player_.IsDead())
	{
		StartPlayerDeath();
	}
	else
	{
		audioManager_.PlaySoundEffect(SoundEffect::PlayerDamage);
	}
}

void Application::StartPlayerDeath()
{
	if (playerDeathWaitTimer_ > 0)
	{
		return;
	}
	audioManager_.StopBgm();
	audioManager_.PlaySoundEffect(SoundEffect::PlayerDeath);

	const float effectDrawSize = 64.0f * 1.5f;
	Vector2 effectPosition{};
	effectPosition.x =
		player_.GetPosition().x + player_.GetWidth() * 0.5f - effectDrawSize * 0.5f;
	effectPosition.y =
		player_.GetPosition().y + player_.GetHeight() * 0.5f - effectDrawSize * 0.5f;

	effectManager_.PlayPlayerDestroyEffect(effectPosition);
	StartScreenShake(PlayerDeathScreenShakeFrames, PlayerDeathScreenShakePower);
	selectedDeathMenuItem_ = 0;
	playerDeathWaitTimer_ = PlayerDeathWaitFrames;
}

bool Application::IsPlayerCollidingWithBoss() const
{
	if (!boss_.IsActive())
	{
		return false;
	}

	const float playerLeft = player_.GetPosition().x;
	const float playerTop = player_.GetPosition().y;
	const float playerRight = playerLeft + player_.GetWidth();
	const float playerBottom = playerTop + player_.GetHeight();

	const float bossLeft = boss_.GetPosition().x;
	const float bossTop = boss_.GetPosition().y;
	const float bossRight = bossLeft + boss_.GetWidth();
	const float bossBottom = bossTop + boss_.GetHeight();

	return
		playerLeft < bossRight &&
		playerRight > bossLeft &&
		playerTop < bossBottom &&
		playerBottom > bossTop;
}

// 現在のステージ条件を見て、クリアなら次に進める。
void Application::CheckStageClear()
{
	const StageData& currentStage =
		stageManager_.GetCurrentStage();

	if (stageClearWaitTimer_ > 0)
	{
		return;
	}

	// ゴール到達型のステージでは、プレイヤーがゴールに触れたら次のステージへ進む。
	if (currentStage.clearConditionType == ClearConditionType::ReachGoal &&
		goal_.IsPlayerReached(player_))
	{
		audioManager_.StopBgm();
		stageClearWaitTimer_ = StageClearWaitFrames;

		return;
	}
}

bool Application::IsStageStarting() const
{
	return stageStartTimer_ > 0;
}

bool Application::IsStageClearWaiting() const
{
	return stageClearWaitTimer_ > 0 || bossClearWaitTimer_ > 0;
}

void Application::StartScreenShake(int duration, int power)
{
	// すでに強い揺れが発生している場合は、弱い揺れで上書きしない。
	if (power < screenShakePower_)
	{
		return;
	}

	// 同じ強さなら、残り時間が短くなる上書きはしない。
	if (power == screenShakePower_ && duration <= screenShakeTimer_)
	{
		return;
	}

	screenShakeTimer_ = duration;
	screenShakePower_ = power;
	screenShakeDirectionX_ = 0;
}

void Application::StartDirectionalScreenShake(int duration, int power, int directionX)
{
	// 通常揺れと同じルールで、強い揺れを弱い揺れで消さない。
	if (power < screenShakePower_)
	{
		return;
	}

	if (power == screenShakePower_ && duration <= screenShakeTimer_)
	{
		return;
	}

	screenShakeTimer_ = duration;
	screenShakePower_ = power;
	screenShakeDirectionX_ = directionX;
}

void Application::UpdateScreenShake()
{
	if (screenShakeTimer_ <= 0)
	{
		return;
	}

	--screenShakeTimer_;

	if (screenShakeTimer_ <= 0)
	{
		screenShakePower_ = 0;
		screenShakeDirectionX_ = 0;
	}
}

void Application::GetScreenShakeOffset(int& offsetX, int& offsetY) const
{
	offsetX = 0;
	offsetY = 0;

	if (screenShakeTimer_ <= 0 || screenShakePower_ <= 0)
	{
		return;
	}

	if (screenShakeDirectionX_ != 0)
	{
		const int directionSign = screenShakeTimer_ % 2 == 0 ? 1 : -1;
		offsetX = screenShakeDirectionX_ * screenShakePower_ * directionSign;
		return;
	}

	const int pattern = screenShakeTimer_ % 4;

	switch (pattern)
	{
	case 0:
		offsetX = screenShakePower_;
		offsetY = 0;
		break;

	case 1:
		offsetX = -screenShakePower_;
		offsetY = 0;
		break;

	case 2:
		offsetX = 0;
		offsetY = screenShakePower_;
		break;

	default:
		offsetX = 0;
		offsetY = -screenShakePower_;
		break;
	}
}

void Application::UpdateCamera()
{
	const int cameraYBeforeUpdate = cameraY_;
	const float playerCenterX =
		player_.GetPosition().x + player_.GetWidth() * 0.5f;
	const float playerCenterY =
		player_.GetPosition().y + player_.GetHeight() * 0.5f;
	const float playerTop = player_.GetPosition().y;
	const float playerBottom = playerTop + player_.GetHeight();
	float cameraSpanTop = playerTop;
	float cameraSpanBottom = playerBottom;

	// 基本の目標位置は従来通りプレイヤー中心にする。
	int targetX =
		static_cast<int>(playerCenterX) - CameraConfig::ViewWidth / 2;
	int targetY =
		static_cast<int>(playerCenterY) -
		CameraConfig::ViewHeight / 2 -
		CameraVerticalOffset;
	int activeAreaIndex = -1;
	CameraAreaMode activeMode = CameraAreaMode::Free;
	CameraArea activeAreaBounds{};
	bool hasActiveArea = false;

	// 後から登録した範囲を優先し、指定された軸だけ追従を止める。
	const StageData& stage = stageManager_.GetCurrentStage();
	const float supportHalfWidth = player_.GetWidth() * 0.33f;
	bool isInsideEditedFallHoleColumn = false;
	for (int i = 0; i < stage.fallHoleCount; ++i)
	{
		const FallHole& hole = stage.fallHoles[i];
		if (!hole.IsLegacyBoundaryHole() &&
			hole.ContainsHorizontal(
				playerCenterX - supportHalfWidth,
				playerCenterX + supportHalfWidth))
		{
			// 穴ワープを遅らせた距離と同じだけカメラ範囲を延長し、
			// ワープ直前だけ通常追従へ戻って画面が跳ねるのを防ぐ。
			isInsideEditedFallHoleColumn = true;
			cameraSpanTop -= StageData::FallLoopTravelMargin;
			cameraSpanBottom += StageData::FallLoopTravelMargin;
			break;
		}
	}
	for (int i = stage.cameraAreaCount - 1; i >= 0; --i)
	{
		const CameraArea& area = stage.cameraAreas[i];
		if (!area.OverlapsVerticalSpan(
			playerCenterX, cameraSpanTop, cameraSpanBottom))
		{
			continue;
		}
		activeAreaIndex = area.groupId >= 0 ? area.groupId : i;
		activeMode = area.mode;
		activeAreaBounds = stage.GetCameraAreaGroupBounds(i);
		hasActiveArea = true;

		if (area.mode == CameraAreaMode::Vertical ||
			area.mode == CameraAreaMode::Locked)
		{
			targetX = (activeAreaBounds.left + activeAreaBounds.right) / 2 -
				CameraConfig::ViewWidth / 2;
		}
		if (area.mode == CameraAreaMode::Horizontal ||
			area.mode == CameraAreaMode::Locked)
		{
			targetY = (activeAreaBounds.top + activeAreaBounds.bottom) / 2 -
				CameraConfig::ViewHeight / 2 - CameraVerticalOffset;
		}
		targetY = activeAreaBounds.ClampViewTopY(
			targetY, CameraConfig::ViewHeight, StageData::TileSize);
		break;
	}
	if (isInsideEditedFallHoleColumn)
	{
		// 参考動画のように中央付近では縦位置を保ち、画面端へ
		// 近づいた時だけ追従してプレイヤーを表示範囲内に残す。
		const float safeTop = static_cast<float>(
			cameraYBeforeUpdate + FallHoleCameraVerticalMargin);
		const float safeBottom = static_cast<float>(
			cameraYBeforeUpdate + CameraConfig::ViewHeight -
			FallHoleCameraVerticalMargin);

		if (playerTop < safeTop)
		{
			targetY = static_cast<int>(playerTop) -
				FallHoleCameraVerticalMargin;
		}
		else if (playerBottom > safeBottom)
		{
			targetY = static_cast<int>(playerBottom) +
				FallHoleCameraVerticalMargin - CameraConfig::ViewHeight;
		}
		else
		{
			targetY = cameraYBeforeUpdate;
		}
	}

	cameraTransition_.Update(targetX, targetY, activeAreaIndex, activeMode);
	cameraX_ = cameraTransition_.GetX();
	cameraY_ = cameraTransition_.GetY();
	if (hasActiveArea)
	{
		// 遷移中のカメラも、遷移先と同じ上下範囲から外れないようにする。
		cameraY_ = activeAreaBounds.ClampViewTopY(
			cameraY_, CameraConfig::ViewHeight, StageData::TileSize);
	}
}

void Application::CheckCheckpointActivation()
{
	const StageData& stage = stageManager_.GetCurrentStage();
	const float playerLeft = player_.GetPosition().x;
	const float playerTop = player_.GetPosition().y;
	const float playerRight = playerLeft + player_.GetWidth();
	const float playerBottom = playerTop + player_.GetHeight();

	int touchedIndex = -1;
	for (int i = 0; i < stage.checkpointCount; ++i)
	{
		const Vector2& checkpoint = stage.checkpoints[i];
		const float checkpointRight = checkpoint.x + StageData::TileSize;
		const float checkpointBottom = checkpoint.y + StageData::TileSize;
		if (playerLeft >= checkpointRight || playerRight <= checkpoint.x ||
			playerTop >= checkpointBottom || playerBottom <= checkpoint.y)
		{
			continue;
		}
		touchedIndex = i;
		break;
	}

	if (touchedIndex < 0)
	{
		touchingCheckpointIndex_ = -1;
		return;
	}
	if (touchingCheckpointIndex_ == touchedIndex)
	{
		return;
	}

	touchingCheckpointIndex_ = touchedIndex;
	activeCheckpointIndex_ = touchedIndex;
	checkpointRespawnPosition_ = player_.GetPosition();
	checkpointGravityDirection_ = gravitySystem_.GetDirection();
	hasCheckpointRespawnPosition_ = true;
	PlayCheckpointEffect(touchedIndex);
	audioManager_.PlaySoundEffect(SoundEffect::Checkpoint);
}

void Application::PlayCheckpointEffect(int checkpointIndex)
{
	const StageData& stage = stageManager_.GetCurrentStage();
	if (checkpointIndex < 0 || checkpointIndex >= stage.checkpointCount)
	{
		return;
	}

	constexpr float EffectSize = 128.0f;
	constexpr float EffectOffsetY = 100.0f;
	const Vector2& checkpointPosition = stage.checkpoints[checkpointIndex];
	const Vector2 effectPosition
	{
		checkpointPosition.x + StageData::TileSize * 0.5f - EffectSize * 0.5f,
		checkpointPosition.y - EffectOffsetY
	};

	effectManager_.PlayCheckpointEffect(effectPosition);
}

void Application::RespawnAtCheckpoint()
{
	const GravityDirection previousDirection = gravitySystem_.GetDirection();
	gravitySystem_.SetDirection(checkpointGravityDirection_);
	const bool gravityDirectionChanged =
		previousDirection != checkpointGravityDirection_;
	if (gravityDirectionChanged)
	{
		enemyManager_.OnGravityReversed();
		boss_.OnGravityReversed();
	}

	player_.Initialize(checkpointRespawnPosition_);
	if (checkpointGravityDirection_ == GravityDirection::Up)
	{
		player_.OnGravityReversed();
	}

	bulletManager_.Clear();
	enemyBulletManager_.Clear();
	boss_.ResetAttackProjectiles();
	laser_.Reset();
	effectManager_.Clear();
	PlayCheckpointEffect(activeCheckpointIndex_);
	playerDeathWaitTimer_ = 0;
	touchingCheckpointIndex_ = -1;
	screenShakeTimer_ = 0;
	screenShakePower_ = 0;
	screenShakeDirectionX_ = 0;
	cameraTransition_.Reset();
	UpdateCamera();
	if (gravityDirectionChanged)
	{
		gravityBlockManager_.OnGravityReversed(
			static_cast<float>(cameraX_),
			static_cast<float>(cameraY_),
			static_cast<float>(CameraConfig::ViewWidth),
			static_cast<float>(CameraConfig::ViewHeight));
	}
	if (boss_.HasEncounterStarted() && boss_.IsActive())
	{
		audioManager_.PlayBossBgm();
	}
	else
	{
		audioManager_.PlayStageBgm(
			stageManager_.GetCurrentStage().stageNumber);
	}
}

void Application::StartCurrentStage()
{
	stageRenderer_.ResetAnimation();
	bossClearWaitTimer_ = 0;
	stageClearWaitTimer_ = 0;
	stageStartTimer_ = StageStartDisplayFrames;
	playerDeathWaitTimer_ = 0;
	isRetryStart_ = false;
	screenShakeTimer_ = 0;
	screenShakePower_ = 0;
	screenShakeDirectionX_ = 0;
	cameraX_ = 0;
	cameraY_ = 0;
	cameraTransition_.Reset();
	sawHazard_.Reset();

	// 次ステージ開始時は、プレイヤー・敵・ゴールを初期位置へ戻す。
	const StageData& currentStage =
		stageManager_.GetCurrentStage();
	audioManager_.PlayStageBgm(currentStage.stageNumber);

	// ステージ開始時は、重力方向も下向きへ戻す。
	gravitySystem_.Reset();

	// ステージ開始時は、プレイヤーのHPと位置を初期状態へ戻す。
	if (currentStage.hasPlayerStartPosition)
	{
		player_.Initialize(currentStage.playerStartPosition);
	}
	else
	{
		player_.Initialize();
	}
	activeCheckpointIndex_ = -1;
	touchingCheckpointIndex_ = -1;
	checkpointRespawnPosition_ = player_.GetPosition();
	checkpointGravityDirection_ = GravityDirection::Down;
	hasCheckpointRespawnPosition_ = true;

	// 前ステージから残っている弾やレーザー状態を消す。
	bulletManager_.Clear();
	laser_.Reset();
	// 前ステージから飛んでいる敵弾も消す。
	enemyBulletManager_.Clear();
	effectManager_.Clear();

	// 敵と、ゴール到達型ステージのゴールを配置し直す。
	enemyManager_.Initialize(currentStage);

	if (currentStage.clearConditionType == ClearConditionType::ReachGoal)
	{
		if (currentStage.hasGoalPosition)
		{
			goal_.Initialize(currentStage.goalPosition);
		}
		else
		{
			goal_.Initialize();
		}
	}

	// ボスは必要なステージにだけ出現させる。
	boss_.Deactivate();

	// 現在のステージ番号に合わせて、重力ブロックを配置する。
	gravityBlockManager_.Initialize(currentStage);

	if (currentStage.hasMidBossSpawn)
	{
		boss_.InitializeMidBoss(currentStage.midBossSpawn);
	}
	else if (currentStage.hasBossSpawn)
	{
		boss_.InitializeBoss(currentStage.bossSpawn);
	}
	gravityRecoveryDevice_.Initialize(currentStage);

	// 開始表示中もプレイヤーが画面中央に見えるよう、初期位置からカメラを決める。
	UpdateCamera();
}

void Application::CheckBossActivation()
{
	const StageData& stage = stageManager_.GetCurrentStage();
	if (boss_.HasEncounterStarted())
	{
		return;
	}
	const int bossCameraGroupId = stage.hasMidBossSpawn
		? stage.midBossSpawn.cameraGroupId
		: stage.hasBossSpawn ? stage.bossSpawn.cameraGroupId : -1;
	if (bossCameraGroupId < 0)
	{
		return;
	}

	const float playerCenterX = player_.GetPosition().x + player_.GetWidth() * 0.5f;
	const float playerCenterY = player_.GetPosition().y + player_.GetHeight() * 0.5f;
	for (int i = 0; i < stage.cameraAreaCount; ++i)
	{
		const CameraArea& area = stage.cameraAreas[i];
		const int groupId = area.groupId >= 0 ? area.groupId : i;
		if (groupId == bossCameraGroupId &&
			area.Contains(playerCenterX, playerCenterY))
		{
			boss_.StartEncounter();
			audioManager_.PlayBossBgm();
			return;
		}
	}
}

void Application::StartEditorPreview(bool fromCursor)
{
	StageData previewStage{};
	if (fromCursor)
	{
		if (!stageEditor_.TryPrepareCursorPreview(
			player_.GetWidth(), player_.GetHeight(), previewStage))
		{
			return;
		}
	}
	else
	{
		previewStage = stageEditor_.PrepareForPreview();
	}

	// コピーをゲーム側へ渡すので、エディターの開始地点は変わらない。
	stageManager_.StartPreview(previewStage);
	isEditorPreviewActive_ = true;
	scoreManager_.Reset();
	StartCurrentStage();
	gameState_ = GameState::Playing;
}

void Application::ReturnToStageEditor()
{
	audioManager_.StopBgm();
	bulletManager_.Clear();
	laser_.Reset();
	enemyBulletManager_.Clear();
	effectManager_.Clear();
	isPauseControlsOpen_ = false;
	stageEditor_.ResumeAfterPreview();
	isEditorPreviewActive_ = false;
	gameState_ = GameState::StageEditor;
}

void Application::GoToNextStage()
{
	if (isEditorPreviewActive_)
	{
		ReturnToStageEditor();
		return;
	}
	stageManager_.GoToNextStage();

	if (stageManager_.IsGameCleared())
	{
		audioManager_.StopBgm();
		displayedGameClearScore_ = 0;
		gameState_ = GameState::GameClear;
		return;
	}

	StartCurrentStage();
}

bool Application::HandleDebugStageJump()
{
	const int debugStageKeys[StageSelectItemCount]
	{
		KEY_INPUT_F1,
		KEY_INPUT_F2,
		KEY_INPUT_F3,
		KEY_INPUT_F4,
		KEY_INPUT_F5
	};

	const int debugStageNumberKeys[StageSelectItemCount]
	{
		KEY_INPUT_1,
		KEY_INPUT_2,
		KEY_INPUT_3,
		KEY_INPUT_4,
		KEY_INPUT_5
	};

	for (int i = 0; i < StageSelectItemCount; ++i)
	{
		if (!inputManager_.IsTriggered(debugStageKeys[i]) &&
			!inputManager_.IsTriggered(debugStageNumberKeys[i]))
		{
			continue;
		}

		const int stageNumber = i + 1;

		// デバッグ確認用なので、選んだステージを初期状態から始め直す。
		stageManager_.StartStage(stageNumber);
		scoreManager_.Reset();
		StartCurrentStage();
		isTitleControlsOpen_ = false;
		isTitleStageSelectOpen_ = false;
		isPauseControlsOpen_ = false;
		gameState_ = GameState::Playing;

		return true;
	}

	return false;
}

void Application::MoveMenuSelection(int& selectedItem, int itemCount)
{
	bool didMove = false;

	// 上キーで1つ上の項目へ移動する。先頭より上へ行ったら最後の項目へ回り込む。
	if (inputManager_.IsTriggered(KEY_INPUT_UP))
	{
		--selectedItem;
		didMove = true;

		if (selectedItem < 0)
		{
			selectedItem = itemCount - 1;
		}
	}

	// 下キーで1つ下の項目へ移動する。最後より下へ行ったら先頭の項目へ回り込む。
	if (inputManager_.IsTriggered(KEY_INPUT_DOWN))
	{
		++selectedItem;
		didMove = true;

		if (selectedItem >= itemCount)
		{
			selectedItem = 0;
		}
	}

	if (didMove)
	{
		audioManager_.PlaySoundEffect(SoundEffect::MenuMove);
	}
}

void Application::DrawScreenPanel(
	int left,
	int top,
	int right,
	int bottom,
	int borderColor) const
{
	// 画面メニューやリザルトで使う、黒背景＋枠線の共通パネル。
	DrawBox(
		left,
		top,
		right,
		bottom,
		GetColor(0, 0, 0),
		TRUE
	);

	DrawBox(
		left,
		top,
		right,
		bottom,
		borderColor,
		FALSE
	);
}

void Application::DrawMenuItem(
	int itemY,
	const wchar_t* text,
	bool isSelected) const
{
	constexpr int ButtonWidth = 320;
	constexpr int ButtonHeight = 64;
	constexpr int MenuFontSize = 22;
	constexpr int ButtonLeft =
		(GameConfig::ScreenWidth - ButtonWidth) / 2;
	constexpr int ButtonRight = ButtonLeft + ButtonWidth;
	constexpr int SourceLeft = 6;
	constexpr int SourceTop = 6;
	constexpr int SourceWidth = 53;
	constexpr int SourceHeight = 15;
	const int buttonTop = itemY - 23;
	const int buttonBottom = buttonTop + ButtonHeight;
	const int textColor =
		isSelected ? GetColor(255, 240, 80) : GetColor(220, 220, 220);
	const int textWidth = UiFont::GetStringWidth(text, MenuFontSize);
	const int textX =
		ButtonLeft + (ButtonRight - ButtonLeft - textWidth) / 2;

	if (menuButtonImageHandle_ != -1)
	{
		SetDrawBlendMode(
			DX_BLENDMODE_ALPHA,
			isSelected ? 255 : 205
		);
		DrawRectExtendGraph(
			ButtonLeft,
			buttonTop,
			ButtonRight,
			buttonBottom,
			SourceLeft,
			SourceTop,
			SourceWidth,
			SourceHeight,
			menuButtonImageHandle_,
			TRUE
		);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	if (isSelected)
	{
		if ((menuCursorBlinkTimer_ / 30) % 2 == 0)
		{
			UiFont::DrawStringSized(
				ButtonLeft - 28,
				itemY,
				L">",
				GetColor(255, 240, 80),
				MenuFontSize
			);
		}
	}

	UiFont::DrawStringSized(
		textX,
		itemY,
		text,
		textColor,
		MenuFontSize
	);
}

void Application::DrawCenteredUiText(
	int y,
	const wchar_t* text,
	unsigned int color,
	int fontSize) const
{
	const int textWidth = UiFont::GetStringWidth(text, fontSize);
	UiFont::DrawStringSized(
		(GameConfig::ScreenWidth - textWidth) / 2,
		y,
		text,
		color,
		fontSize
	);
}

void Application::DrawTitleBackground() const
{
	DrawBox(
		0,
		0,
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		GetColor(4, 7, 24),
		TRUE
	);

	if (titleStarImageHandle_ == -1)
	{
		return;
	}

	struct TitleStar
	{
		int x;
		int y;
		int size;
		int phase;
	};

	constexpr TitleStar Stars[] =
	{
		{  65,  70, 48, 0 }, { 155, 175, 32, 1 },
		{ 250,  75, 28, 2 }, { 345, 245, 40, 3 },
		{ 455,  65, 36, 1 }, { 585, 225, 28, 0 },
		{ 710,  80, 52, 3 }, { 840, 205, 32, 2 },
		{ 970,  70, 40, 1 }, { 1100, 165, 28, 0 },
		{ 1215, 65, 36, 2 }, {  85, 330, 32, 3 },
		{ 250, 400, 48, 0 }, { 345, 610, 28, 2 },
		{ 1040, 335, 52, 1 }, { 1180, 435, 32, 3 },
		{ 1005, 620, 36, 0 }, { 115, 650, 40, 2 },
		{ 735, 660, 28, 1 }, { 930, 545, 32, 3 },
		{ 1100, 570, 24, 1 }, {  50, 520, 28, 1 }
	};

	constexpr int FrameSize = 32;
	constexpr int FrameCount = 4;
	constexpr int FrameDuration = 9;
	constexpr int DisplayScalePercent = 50;
	const int baseFrame = menuCursorBlinkTimer_ / FrameDuration;
	const int scrollX =
		menuCursorBlinkTimer_ % GameConfig::ScreenWidth;

	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	for (const TitleStar& star : Stars)
	{
		const int frame = (baseFrame + star.phase) % FrameCount;
		const int drawSize = star.size * DisplayScalePercent / 100;
		const int halfSize = drawSize / 2;
		const int drawX =
			(star.x - scrollX + GameConfig::ScreenWidth) %
			GameConfig::ScreenWidth;
		const auto drawStar = [&](int x)
		{
			DrawRectExtendGraph(
				x - halfSize,
				star.y - halfSize,
				x + halfSize,
				star.y + halfSize,
				frame * FrameSize,
				0,
				FrameSize,
				FrameSize,
				titleStarImageHandle_,
				TRUE
			);
		};

		drawStar(drawX);
		if (drawX - halfSize < 0)
		{
			drawStar(drawX + GameConfig::ScreenWidth);
		}
		else if (drawX + halfSize > GameConfig::ScreenWidth)
		{
			drawStar(drawX - GameConfig::ScreenWidth);
		}
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Application::DrawTitleScreen()
{
	DrawTitleBackground();

	constexpr wchar_t TitleText[] = L"GRAVITY SHIFT";
	constexpr int TitleFontSize = 64;
	const int titleWidth =
		UiFont::GetStringWidth(TitleText, TitleFontSize);
	const int titleX = (GameConfig::ScreenWidth - titleWidth) / 2;
	constexpr int TitleY = 110;
	constexpr int OutlineOffsets[][2] =
	{
		{ -3,  0 }, { 3,  0 }, {  0, -3 }, {  0, 3 },
		{ -2, -2 }, { 2, -2 }, { -2,  2 }, {  2, 2 }
	};
	for (const auto& offset : OutlineOffsets)
	{
		UiFont::DrawStringSized(
			titleX + offset[0],
			TitleY + offset[1],
			TitleText,
			GetColor(0, 0, 0),
			TitleFontSize
		);
	}
	UiFont::DrawStringSized(
		titleX,
		TitleY,
		TitleText,
		GetColor(255, 140, 20),
		TitleFontSize
	);

	const wchar_t* menuTexts[TitleMenuItemCount] =
	{
		L"NEW GAME",
		L"STAGE SELECT",
		L"CONTROLS",
		L"STAGE EDITOR",
		L"EXIT"
	};

	for (int i = 0; i < TitleMenuItemCount; ++i)
	{
		const int itemY = 315 + i * 65;
		DrawMenuItem(
			itemY,
			menuTexts[i],
			i == selectedTitleMenuItem_
		);
	}

	DrawCenteredUiText(
		640,
		L"上下キー / 十字キー：選択　ENTER / A：決定",
		GetColor(160, 220, 255)
	);
}

void Application::DrawTitleStageSelect()
{
	// タイトルメニューから開く、テスト用にも使えるステージ選択画面。
	DrawTitleBackground();

	DrawCenteredUiText(
		110,
		isSelectingStageForEditor_ ? L"EDIT STAGE" : L"STAGE SELECT",
		GetColor(255, 255, 255),
		40
	);

	const wchar_t* stageTexts[StageSelectItemCount] =
	{
		L"STAGE 1",
		L"STAGE 2",
		L"STAGE 3",
		L"STAGE 4",
		L"STAGE 5"
	};

	for (int i = 0; i < StageSelectItemCount; ++i)
	{
		const int itemY = 230 + i * 65;
		DrawMenuItem(
			itemY,
			stageTexts[i],
			i == selectedStageSelectItem_
		);
	}

	const wchar_t* stageSelectHelp = isSelectingStageForEditor_
			? L"上下キー / 十字キー：選択　ENTER / A：編集　ESC / B：戻る"
			: L"上下キー / 十字キー：選択　ENTER / A：開始　ESC / B：戻る";
	DrawCenteredUiText(
		570,
		stageSelectHelp,
		GetColor(160, 220, 255)
	);
}

void Application::DrawTitleControls()
{
	// 操作説明画面。タイトルメニューから開き、EnterまたはEscで戻る。
	DrawTitleBackground();

	DrawCenteredUiText(
		110,
		L"操作方法",
		GetColor(255, 255, 255),
		40
	);

	DrawScreenPanel(
		250,
		180,
		1030,
		560,
		GetColor(160, 220, 255)
	);

	UiFont::DrawString(300, 205, L"キーボード / GP20x（XInput）", GetColor(160, 220, 255));
	UiFont::DrawString(300, 245, L"移動・照準：矢印キー / 左スティック・十字キー", GetColor(220, 220, 220));
	UiFont::DrawString(300, 285, L"ダッシュ：SHIFT / X（再使用まで3秒）", GetColor(220, 220, 220));
	UiFont::DrawString(300, 325, L"ジャンプ：SPACE / A", GetColor(220, 220, 220));
	UiFont::DrawString(300, 365, L"重力反転：G / Y", GetColor(220, 220, 220));
	UiFont::DrawString(300, 405, L"射撃：Z / B（上下入力で頭上・足元）", GetColor(220, 220, 220));
	UiFont::DrawString(300, 445, L"レーザー：X / RB", GetColor(220, 220, 220));
	UiFont::DrawString(300, 485, L"ポーズ：P / START", GetColor(220, 220, 220));

	DrawCenteredUiText(
		610,
		L"ENTER / A または ESC / B：メニューへ戻る",
		GetColor(160, 220, 255)
	);
}

void Application::DrawGameClearScreen()
{
	DrawCenteredUiText(
		200,
		L"GAME CLEAR",
		GetColor(255, 240, 80),
		64
	);

	DrawCenteredUiText(
		290,
		L"最終スコア",
		GetColor(160, 220, 255),
		32
	);

	wchar_t scoreText[32]{};
	swprintf_s(
		scoreText,
		_countof(scoreText),
		L"%d",
		displayedGameClearScore_
	);
	DrawCenteredUiText(
		340,
		scoreText,
		GetColor(255, 255, 255),
		48
	);

	DrawCenteredUiText(
		440,
		L"ENTER / A：タイトルへ戻る",
		GetColor(160, 220, 255),
		24
	);

	DrawCenteredUiText(
		485,
		L"ESC：ゲームを終了",
		GetColor(180, 180, 180),
		24
	);
}

void Application::DrawGameOverScreen()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
	DrawBox(
		0,
		0,
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		GetColor(0, 0, 0),
		TRUE
	);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawCenteredUiText(
		210,
		L"PLAYER DOWN",
		GetColor(255, 80, 80),
		40
	);

	const wchar_t* menuTexts[DeathMenuItemCount]
	{
		L"RESTART FROM CHECKPOINT",
		L"RESTART STAGE",
		isEditorPreviewActive_ ? L"RETURN TO EDITOR" : L"RETURN TO TITLE"
	};

	for (int i = 0; i < DeathMenuItemCount; ++i)
	{
		const int itemY = 280 + i * 70;
		DrawMenuItem(
			itemY,
			menuTexts[i],
			i == selectedDeathMenuItem_
		);
	}

	DrawCenteredUiText(
		485,
		L"上下キー / 十字キー：選択　ENTER / A：決定",
		GetColor(160, 220, 255)
	);
}

// 1フレーム分の画面描画を行う。
// 背景・ステージ → キャラクター → UI/デバッグ表示の順に描画する。
void Application::Draw()
{
	ClearDrawScreen();

	// ステージエディターはゲーム本編とは別の画面として描画する。
	if (gameState_ == GameState::StageEditor)
	{
		stageEditor_.Draw();
		ScreenFlip();
		return;
	}

	// タイトル画面。
	if (gameState_ == GameState::Title)
	{
		if (isTitleControlsOpen_)
		{
			DrawTitleControls();
		}
		else if (isTitleStageSelectOpen_)
		{
			DrawTitleStageSelect();
		}
		else
		{
			DrawTitleScreen();
		}

		ScreenFlip();
		return;
	}

	// ゲームクリア画面。
	if (gameState_ == GameState::GameClear)
	{
		DrawGameClearScreen();
		ScreenFlip();
		return;
	}

	SetDrawScreen(worldScreenHandle_);
	ClearDrawScreen();
	const MATRIX cameraTransform = MGetTranslate(VGet(
		static_cast<float>(-cameraX_),
		static_cast<float>(-cameraY_),
		0.0f));
	SetTransformTo2D(&cameraTransform);
	DrawGameWorld();
	const MATRIX identityTransform = MGetIdent();
	SetTransformTo2D(&identityTransform);

	int screenShakeOffsetX = 0;
	int screenShakeOffsetY = 0;
	GetScreenShakeOffset(screenShakeOffsetX, screenShakeOffsetY);

	SetDrawScreen(gameScreenHandle_);
	ClearDrawScreen();

	DrawExtendGraph(
		0,
		0,
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		worldScreenHandle_,
		FALSE
	);

	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();
	DrawGraph(screenShakeOffsetX, screenShakeOffsetY, gameScreenHandle_, FALSE);

	DrawGameUi();

	if (isDebugGridVisible_)
	{
		DrawDebugGrid();
	}

	if (gameState_ == GameState::Pause)
	{
		DrawPauseOverlay();
	}
	else if (gameState_ == GameState::GameOver)
	{
		DrawGameOverScreen();
	}

	ScreenFlip();
}

// ステージやキャラクターなど、画面揺れの対象になる部分を描画する。
void Application::DrawGameWorld()
{
	const StageData& currentStage =
		stageManager_.GetCurrentStage();

	stageRenderer_.Draw(currentStage);
	sawHazard_.Draw(currentStage);
	gravityBlockManager_.Draw();
	DrawCheckpoints();
	gravityRecoveryDevice_.Draw();
	player_.Draw();
	enemyManager_.Draw();
	enemyBulletManager_.Draw();
	boss_.Draw();
	if (currentStage.clearConditionType == ClearConditionType::ReachGoal)
	{
		goal_.Draw();
	}
	bulletManager_.Draw();
	laser_.Draw();
	effectManager_.Draw();
	scoreManager_.DrawPopups();
}

void Application::DrawCheckpoints() const
{
	const StageData& stage = stageManager_.GetCurrentStage();
	for (int i = 0; i < stage.checkpointCount; ++i)
	{
		const Vector2& position = stage.checkpoints[i];
		const int centerX = static_cast<int>(position.x) + StageData::TileSize / 2;
		const int top = static_cast<int>(position.y) + 4;
		const int bottom = static_cast<int>(position.y) + StageData::TileSize - 4;
		const bool isActive = i == activeCheckpointIndex_;
		if (checkpointImageHandle_ != -1)
		{
			const int drawLeft = static_cast<int>(position.x) + 4;
			const int drawTop = static_cast<int>(position.y) + 4;
			if (isActive)
			{
				SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
				DrawGraph(drawLeft, drawTop, checkpointImageHandle_, TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			}
			DrawGraph(drawLeft, drawTop, checkpointImageHandle_, TRUE);
			continue;
		}

		const int color = isActive
			? GetColor(255, 235, 90)
			: GetColor(80, 220, 255);

		DrawLine(centerX, top + 8, centerX, bottom, color, 3);
		DrawCircle(centerX, top + 7, 7, color, FALSE, 2);
		if (isActive)
		{
			DrawCircle(centerX, top + 7, 3, color, TRUE);
		}
		DrawLine(centerX - 8, bottom, centerX + 8, bottom, color, 3);
	}
}

void Application::DrawDebugGrid()
{
	const int gridColor = GetColor(60, 120, 140);
	const int axisColor = GetColor(120, 220, 255);
	const int textColor = GetColor(255, 255, 255);
	const int panelColor = GetColor(0, 0, 0);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);

	const int firstWorldGridX =
		(cameraX_ / StageData::TileSize) * StageData::TileSize;
	const int firstWorldGridY =
		(cameraY_ / StageData::TileSize) * StageData::TileSize;

	for (int worldX = firstWorldGridX;
		worldX <= cameraX_ + CameraConfig::ViewWidth;
		worldX += StageData::TileSize)
	{
		const int screenX =
			(worldX - cameraX_) * GameConfig::ScreenWidth /
			CameraConfig::ViewWidth;

		DrawLine(
			screenX,
			0,
			screenX,
			GameConfig::ScreenHeight,
			gridColor
		);
	}

	for (int worldY = firstWorldGridY;
		worldY <= cameraY_ + CameraConfig::ViewHeight;
		worldY += StageData::TileSize)
	{
		const int screenY =
			(worldY - cameraY_) * GameConfig::ScreenHeight /
			CameraConfig::ViewHeight;

		DrawLine(
			0,
			screenY,
			GameConfig::ScreenWidth,
			screenY,
			gridColor
		);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawLine(
		0,
		(GameConfig::CeilingY - cameraY_) *
			GameConfig::ScreenHeight / CameraConfig::ViewHeight,
		GameConfig::ScreenWidth,
		(GameConfig::CeilingY - cameraY_) *
			GameConfig::ScreenHeight / CameraConfig::ViewHeight,
		axisColor
	);

	DrawLine(
		0,
		(GameConfig::FloorY - cameraY_) *
			GameConfig::ScreenHeight / CameraConfig::ViewHeight,
		GameConfig::ScreenWidth,
		(GameConfig::FloorY - cameraY_) *
			GameConfig::ScreenHeight / CameraConfig::ViewHeight,
		axisColor
	);

	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);

	const int worldMouseX = cameraX_ +
		mouseX * CameraConfig::ViewWidth / GameConfig::ScreenWidth;
	const int worldMouseY = cameraY_ +
		mouseY * CameraConfig::ViewHeight / GameConfig::ScreenHeight;
	const int tileX = worldMouseX / StageData::TileSize;
	const int tileY = worldMouseY / StageData::TileSize;
	const int snappedX = tileX * StageData::TileSize;
	const int snappedY = tileY * StageData::TileSize;

	DrawBox(
		900,
		20,
		1260,
		280,
		panelColor,
		TRUE
	);

	DrawBox(
		900,
		20,
		1260,
		280,
		axisColor,
		FALSE
	);

	UiFont::DrawString(
		915,
		35,
		L"DEBUG GRID: 9 OFF",
		textColor
	);

	UiFont::DrawFormatString(
		915,
		62,
		textColor,
		L"Pixel: %d, %d",
		worldMouseX,
		worldMouseY
	);

	UiFont::DrawFormatString(
		915,
		88,
		textColor,
		L"Snap : %d, %d",
		snappedX,
		snappedY
	);

	UiFont::DrawFormatString(
		915,
		114,
		textColor,
		L"Tile : %d, %d",
		tileX,
		tileY
	);

	UiFont::DrawFormatString(
		915,
		144,
		textColor,
		L"Map row: %d",
		tileY
	);

	UiFont::DrawFormatString(
		915,
		170,
		textColor,
		L"Map col: %d",
		tileX
	);

	UiFont::DrawFormatString(
		915,
		200,
		textColor,
		L"Enemy: Normal %d %d 1 Floor",
		snappedX,
		snappedY
	);

	UiFont::DrawFormatString(
		915,
		226,
		textColor,
		L"Block: %d %d",
		snappedX,
		snappedY
	);

	UiFont::DrawFormatString(
		915,
		252,
		textColor,
		L"Goal : %d %d",
		snappedX,
		snappedY
	);
}

void Application::DrawPauseOverlay()
{
	// ゲーム画面の上に半透明の暗いパネルを重ねて、停止中であることを分かりやすくする。
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);

	DrawBox(
		0,
		0,
		GameConfig::ScreenWidth,
		GameConfig::ScreenHeight,
		GetColor(0, 0, 0),
		TRUE
	);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawCenteredUiText(
		220,
		L"PAUSE",
		GetColor(255, 255, 255),
		40
	);

	if (isPauseControlsOpen_)
	{
		UiFont::DrawString(365, 280, L"移動・照準：矢印キー / 左スティック・十字キー", GetColor(220, 220, 220));
		UiFont::DrawString(365, 315, L"ダッシュ：SHIFT / X（再使用まで3秒）", GetColor(220, 220, 220));
		UiFont::DrawString(365, 350, L"ジャンプ：SPACE / A", GetColor(220, 220, 220));
		UiFont::DrawString(365, 385, L"重力反転：G / Y", GetColor(220, 220, 220));
		UiFont::DrawString(365, 420, L"射撃：Z / B（上下入力で頭上・足元）", GetColor(220, 220, 220));
		UiFont::DrawString(365, 455, L"レーザー：X / RB", GetColor(220, 220, 220));
		UiFont::DrawString(365, 490, L"ポーズ：P / START", GetColor(220, 220, 220));

		const wchar_t* pauseControlsHelp = isEditorPreviewActive_
			? L"ENTER / A / B：ポーズへ戻る　ESC：エディターへ戻る"
			: L"ENTER / A または ESC / B：ポーズへ戻る";
		DrawCenteredUiText(
			560,
			pauseControlsHelp,
			GetColor(160, 220, 255)
		);

		return;
	}

	const wchar_t* menuTexts[PauseMenuItemCount] =
	{
		L"RESUME",
		L"CONTROLS",
		isEditorPreviewActive_ ? L"EDITOR" : L"TITLE",
		L"EXIT"
	};

	for (int i = 0; i < PauseMenuItemCount; ++i)
	{
		const int itemY = 285 + i * 65;
		DrawMenuItem(
			itemY,
			menuTexts[i],
			i == selectedPauseMenuItem_
		);
	}

	DrawCenteredUiText(
		560,
		L"上下キー / 十字キー：選択　ENTER / A：決定　P / START / B：再開",
		GetColor(160, 220, 255)
	);
}

// HPやステージ番号など、画面揺れさせないUIを描画する。
void Application::DrawGameUi()
{
	const StageData& currentStage =
		stageManager_.GetCurrentStage();

	float laserRatio = 1.0f;

	if (laser_.IsActive())
	{
		laserRatio = laser_.GetActiveRatio();
	}
	else if (!laser_.CanFire())
	{
		// クールタイムは残り時間ではなく、回復していく見た目にする。
		laserRatio = 1.0f - laser_.GetCooldownRatio();
	}

	if (laserGaugeImageHandle_ != -1)
	{
		constexpr int laserGaugeX = 20;
		constexpr int laserGaugeY = 20;
		constexpr int laserGaugeScale = 2;
		constexpr int barSourceWidth = 77;
		const int barWidth = ClampInt(
			static_cast<int>(barSourceWidth * laserRatio), 0, barSourceWidth);

		if (barWidth > 0)
		{
			DrawRectExtendGraph(
				laserGaugeX + 40 * laserGaugeScale,
				laserGaugeY + (14 - 7) * laserGaugeScale,
				laserGaugeX + (40 + barWidth) * laserGaugeScale,
				laserGaugeY + (29 - 7) * laserGaugeScale,
				41, 41, barWidth, 15, laserGaugeImageHandle_, TRUE);
		}
		DrawRectExtendGraph(
			laserGaugeX, laserGaugeY,
			laserGaugeX + 128 * laserGaugeScale,
			laserGaugeY + 29 * laserGaugeScale,
			0, 7, 128, 29, laserGaugeImageHandle_, TRUE);
	}
	else
	{
		constexpr int gaugeLeft = 20;
		constexpr int gaugeTop = 20;
		constexpr int gaugeWidth = 200;
		constexpr int gaugeHeight = 18;
		const int filledRight = gaugeLeft +
			ClampInt(static_cast<int>(gaugeWidth * laserRatio), 0, gaugeWidth);
		const int gaugeColor = !laser_.CanFire() && !laser_.IsActive()
			? GetColor(255, 170, 80) : GetColor(80, 255, 255);

		DrawBox(gaugeLeft, gaugeTop,
			gaugeLeft + gaugeWidth, gaugeTop + gaugeHeight,
			GetColor(40, 60, 70), TRUE);
		DrawBox(gaugeLeft, gaugeTop,
			filledRight, gaugeTop + gaugeHeight,
			gaugeColor, TRUE);
		DrawBox(gaugeLeft, gaugeTop,
			gaugeLeft + gaugeWidth, gaugeTop + gaugeHeight,
			GetColor(255, 255, 255), FALSE);
	}

	if (hpGaugeImageHandle_ != -1)
	{
		constexpr int hpGaugeX = 20;
		constexpr int hpGaugeY = 90;
		constexpr int hpGaugeScale = 2;
		constexpr int hpPipCount = 5;

		const int maxHp = player_.GetMaxHitPoint();
		const int hp = ClampInt(player_.GetHitPoint(), 0, maxHp);
		const int litPips = maxHp > 0
			? (hp * hpPipCount + maxHp / 2) / maxHp : 0;

		DrawRectExtendGraph(
			hpGaugeX, hpGaugeY,
			hpGaugeX + 128 * hpGaugeScale,
			hpGaugeY + 29 * hpGaugeScale,
			0, 14, 128, 29, hpGaugeImageHandle_, TRUE);

		if (litPips > 0)
		{
			const int pipSourceWidth = (litPips - 1) * 12 + 10;
			DrawRectExtendGraph(
				hpGaugeX + 41 * hpGaugeScale,
				hpGaugeY + (22 - 14) * hpGaugeScale,
				hpGaugeX + (41 + pipSourceWidth) * hpGaugeScale,
				hpGaugeY + (29 - 14) * hpGaugeScale,
				42, 52, pipSourceWidth, 7,
				hpGaugeImageHandle_, TRUE);
		}
	}
	else
	{
		UiFont::DrawFormatString(
			20, 90, GetColor(255, 255, 120),
			L"HP: %d / %d",
			player_.GetHitPoint(), player_.GetMaxHitPoint());
	}

	constexpr int gravityIndicatorLeft = 28;
	constexpr int gravityIndicatorTop = 160;
	constexpr int gravityIndicatorSize = 24;
	const int gravityIndicatorColor =
		gravitySystem_.GetDirection() == GravityDirection::Down
		? GetColor(255, 70, 70)
		: GetColor(70, 150, 255);
	DrawBox(
		gravityIndicatorLeft,
		gravityIndicatorTop,
		gravityIndicatorLeft + gravityIndicatorSize,
		gravityIndicatorTop + gravityIndicatorSize,
		gravityIndicatorColor,
		TRUE
	);
	DrawBox(
		gravityIndicatorLeft,
		gravityIndicatorTop,
		gravityIndicatorLeft + gravityIndicatorSize,
		gravityIndicatorTop + gravityIndicatorSize,
		GetColor(255, 255, 255),
		FALSE
	);

	if (IsStageStarting())
	{
		DrawBox(
			0,
			250,
			GameConfig::ScreenWidth,
			390,
			GetColor(0, 0, 0),
			TRUE
		);

		if (isRetryStart_)
		{
			UiFont::DrawString(
				570,
				290,
				L"RETRY",
				GetColor(255, 80, 80)
			);

			wchar_t restartText[64]{};
			swprintf_s(
				restartText,
				_countof(restartText),
				L"ステージ%d リスタート",
				currentStage.stageNumber
			);
			DrawCenteredUiText(
				340,
				restartText,
				GetColor(160, 220, 255)
			);
		}
		else
		{
			wchar_t stageText[32]{};
			swprintf_s(
				stageText,
				_countof(stageText),
				L"ステージ%d",
				currentStage.stageNumber
			);
			DrawCenteredUiText(
				290,
				stageText,
				GetColor(255, 240, 80)
			);

			DrawCenteredUiText(
				340,
				L"準備中...",
				GetColor(160, 220, 255)
			);
		}
	}

	if (stageClearWaitTimer_ > 0 || bossClearWaitTimer_ > 0)
	{
		DrawBox(
			0,
			250,
			GameConfig::ScreenWidth,
			390,
			GetColor(0, 0, 0),
			TRUE
		);

		DrawCenteredUiText(
			290,
			L"STAGE CLEAR",
			GetColor(255, 240, 80)
		);

		DrawCenteredUiText(
			340,
			isEditorPreviewActive_
				? L"エディターへ戻ります..."
				: L"次のステージへ...",
			GetColor(160, 220, 255)
		);
	}
}

// DXライブラリが確保したリソースを解放する。
void Application::Finalize()
{
	// 読み込んだ画像を解放してから終了する。
	bulletManager_.Release();
	enemyBulletManager_.Release();
	boss_.Release();
	gravityRecoveryDevice_.Release();
	audioManager_.Release();
	goal_.Release();
	gravityBlockManager_.Release();
	sawHazard_.Release();
	player_.ReleaseSprites();
	UiFont::Release();
	if (hpGaugeImageHandle_ != -1)
	{
		DeleteGraph(hpGaugeImageHandle_);
		hpGaugeImageHandle_ = -1;
	}
	if (laserGaugeImageHandle_ != -1)
	{
		DeleteGraph(laserGaugeImageHandle_);
		laserGaugeImageHandle_ = -1;
	}
	if (checkpointImageHandle_ != -1)
	{
		DeleteGraph(checkpointImageHandle_);
		checkpointImageHandle_ = -1;
	}
	if (titleStarImageHandle_ != -1)
	{
		DeleteGraph(titleStarImageHandle_);
		titleStarImageHandle_ = -1;
	}
	if (menuButtonImageHandle_ != -1)
	{
		DeleteGraph(menuButtonImageHandle_);
		menuButtonImageHandle_ = -1;
	}
	if (gameScreenHandle_ != -1)
	{
		DeleteGraph(gameScreenHandle_);
		gameScreenHandle_ = -1;
	}

	if (worldScreenHandle_ != -1)
	{
		DeleteGraph(worldScreenHandle_);
		worldScreenHandle_ = -1;
	}

	DxLib_End();
}

// 初期化に成功したら、終了要求が出るまでUpdateとDrawを繰り返す。
int Application::Run()
{
	if (!Initialize())
	{
		return -1;
	}

	while (isRunning_)
	{
		Update();

		if(!isRunning_)
		{
			break;
		}

		Draw();
	}

	Finalize();

	return 0;
}
