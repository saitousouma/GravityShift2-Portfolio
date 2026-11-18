#pragma once

// ゲーム内で再生する効果音の種類。
enum class SoundEffect
{
	PlayerJump,
	Dash,
	PlayerShot,
	Laser,
	GravityReverse,
	Heal,
	Checkpoint,
	Restart,
	PlayerDamage,
	PlayerDeath,
	EnemyDamage,
	EnemyDeath,
	BossGrenade,
	BossDeath,
	BossCooldown,
	MenuConfirm,
	MenuMove
};

// BGMと効果音の読み込み、再生、解放をまとめて管理する。
class AudioManager
{
public:
	// 全ての音声を読み込み、音量を設定する。
	bool Load();
	void Release();

	// 画面やステージに応じたBGMへ切り替える。
	void PlayTitleBgm();
	void PlayStageBgm(int stageNumber);
	void PlayBossBgm();
	void StopBgm();
	// 指定された効果音を一度だけ再生する。
	void PlaySoundEffect(SoundEffect soundEffect);

private:
	// 同じBGMの再起動を避けながらループ再生へ切り替える。
	void PlayBgm(int soundHandle);

	int titleBgmHandle_ = -1;
	int stageBgmHandles_[3]{ -1, -1, -1 };
	int bossBgmHandle_ = -1;
	int playerJumpHandle_ = -1;
	int dashHandle_ = -1;
	int playerShotHandle_ = -1;
	int laserHandle_ = -1;
	int gravityReverseHandle_ = -1;
	int healHandle_ = -1;
	int checkpointHandle_ = -1;
	int restartHandle_ = -1;
	int playerDamageHandle_ = -1;
	int playerDeathHandle_ = -1;
	int enemyDamageHandle_ = -1;
	int enemyDeathHandle_ = -1;
	int bossGrenadeHandle_ = -1;
	int bossDeathHandle_ = -1;
	int bossCooldownHandle_ = -1;
	int menuConfirmHandle_ = -1;
	int menuMoveHandle_ = -1;
	int currentBgmHandle_ = -1;
};
