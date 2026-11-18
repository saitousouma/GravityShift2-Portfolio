#include "AudioManager.h"

#include <string>

#include "DxLib.h"

namespace
{
	// 実行場所が異なる場合に備え、二つの基準パスから音声を探す。
	int LoadAudio(const wchar_t* fileName)
	{
		const std::wstring file = fileName;
		int handle = LoadSoundMem((L"Assets/Audio/" + file).c_str());
		if (handle == -1)
		{
			handle = LoadSoundMem((L"../../Assets/Audio/" + file).c_str());
		}
		return handle;
	}

	// 有効な音声ハンドルだけを解放し、未読み込み状態へ戻す。
	void ReleaseAudio(int& handle)
	{
		if (handle == -1)
		{
			return;
		}
		DeleteSoundMem(handle);
		handle = -1;
	}
}

bool AudioManager::Load()
{
	Release();
	// BGMと各ゲーム操作に対応する効果音をまとめて読み込む。
	titleBgmHandle_ = LoadAudio(L"title-bgm.mp3");
	stageBgmHandles_[0] = LoadAudio(L"stage1-bgm.mp3");
	stageBgmHandles_[1] = LoadAudio(L"stage2-bgm.mp3");
	stageBgmHandles_[2] = LoadAudio(L"stage3-bgm.mp3");
	bossBgmHandle_ = LoadAudio(L"boss-bgm.mp3");
	playerJumpHandle_ = LoadAudio(L"player-jump.mp3");
	dashHandle_ = LoadAudio(L"dash.mp3");
	playerShotHandle_ = LoadAudio(L"player-shot.mp3");
	laserHandle_ = LoadAudio(L"laser.wav");
	gravityReverseHandle_ = LoadAudio(L"gravity-reverse.mp3");
	healHandle_ = LoadAudio(L"heal.wav");
	checkpointHandle_ = LoadAudio(L"checkpoint.wav");
	restartHandle_ = LoadAudio(L"restart.mp3");
	playerDamageHandle_ = LoadAudio(L"player-damage.mp3");
	playerDeathHandle_ = LoadAudio(L"player-death.mp3");
	enemyDamageHandle_ = LoadAudio(L"enemy-damage.mp3");
	enemyDeathHandle_ = LoadAudio(L"enemy-death.mp3");
	bossGrenadeHandle_ = LoadAudio(L"boss-grenade.mp3");
	bossDeathHandle_ = LoadAudio(L"boss-death.mp3");
	bossCooldownHandle_ = LoadAudio(L"boss-cooldown.wav");
	menuConfirmHandle_ = LoadAudio(L"menu-confirm.mp3");
	menuMoveHandle_ = LoadAudio(L"menu-move.mp3");

	// BGMと効果音は聞き取りやすさが異なるため、別々の音量にする。
	int* bgmHandles[] = {
		&titleBgmHandle_,
		&stageBgmHandles_[0], &stageBgmHandles_[1],
		&stageBgmHandles_[2], &bossBgmHandle_
	};
	for (int* handle : bgmHandles)
	{
		if (*handle != -1)
		{
			ChangeVolumeSoundMem(150, *handle);
		}
	}

	int* effectHandles[] = {
		&playerJumpHandle_, &dashHandle_, &playerShotHandle_, &laserHandle_,
		&gravityReverseHandle_, &healHandle_, &checkpointHandle_,
		&restartHandle_, &playerDamageHandle_,
		&playerDeathHandle_, &enemyDamageHandle_, &enemyDeathHandle_,
		&bossGrenadeHandle_, &bossDeathHandle_, &bossCooldownHandle_,
		&menuConfirmHandle_, &menuMoveHandle_
	};
	// 一つでも読み込めなかった場合は、呼び出し元へ失敗を通知する。
	bool loadedAll = true;
	for (int* handle : effectHandles)
	{
		loadedAll = loadedAll && *handle != -1;
		if (*handle != -1)
		{
			ChangeVolumeSoundMem(185, *handle);
		}
	}
	for (int* handle : bgmHandles)
	{
		loadedAll = loadedAll && *handle != -1;
	}
	return loadedAll;
}

void AudioManager::Release()
{
	StopBgm();
	ReleaseAudio(titleBgmHandle_);
	for (int& handle : stageBgmHandles_)
	{
		ReleaseAudio(handle);
	}
	ReleaseAudio(bossBgmHandle_);
	ReleaseAudio(playerJumpHandle_);
	ReleaseAudio(dashHandle_);
	ReleaseAudio(playerShotHandle_);
	ReleaseAudio(laserHandle_);
	ReleaseAudio(gravityReverseHandle_);
	ReleaseAudio(healHandle_);
	ReleaseAudio(checkpointHandle_);
	ReleaseAudio(restartHandle_);
	ReleaseAudio(playerDamageHandle_);
	ReleaseAudio(playerDeathHandle_);
	ReleaseAudio(enemyDamageHandle_);
	ReleaseAudio(enemyDeathHandle_);
	ReleaseAudio(bossGrenadeHandle_);
	ReleaseAudio(bossDeathHandle_);
	ReleaseAudio(bossCooldownHandle_);
	ReleaseAudio(menuConfirmHandle_);
	ReleaseAudio(menuMoveHandle_);
}

void AudioManager::PlayTitleBgm()
{
	PlayBgm(titleBgmHandle_);
}

void AudioManager::PlayStageBgm(int stageNumber)
{
	if (stageNumber < 1 || stageNumber > 3)
	{
		StopBgm();
		return;
	}
	PlayBgm(stageBgmHandles_[stageNumber - 1]);
}

void AudioManager::PlayBossBgm()
{
	PlayBgm(bossBgmHandle_);
}

void AudioManager::StopBgm()
{
	if (currentBgmHandle_ != -1)
	{
		StopSoundMem(currentBgmHandle_);
	}
	currentBgmHandle_ = -1;
}

void AudioManager::PlaySoundEffect(SoundEffect soundEffect)
{
	// 列挙値を対応する音声ハンドルへ変換する。
	int handle = -1;
	switch (soundEffect)
	{
	case SoundEffect::PlayerJump: handle = playerJumpHandle_; break;
	case SoundEffect::Dash: handle = dashHandle_; break;
	case SoundEffect::PlayerShot: handle = playerShotHandle_; break;
	case SoundEffect::Laser: handle = laserHandle_; break;
	case SoundEffect::GravityReverse: handle = gravityReverseHandle_; break;
	case SoundEffect::Heal: handle = healHandle_; break;
	case SoundEffect::Checkpoint: handle = checkpointHandle_; break;
	case SoundEffect::Restart: handle = restartHandle_; break;
	case SoundEffect::PlayerDamage: handle = playerDamageHandle_; break;
	case SoundEffect::PlayerDeath: handle = playerDeathHandle_; break;
	case SoundEffect::EnemyDamage: handle = enemyDamageHandle_; break;
	case SoundEffect::EnemyDeath: handle = enemyDeathHandle_; break;
	case SoundEffect::BossGrenade: handle = bossGrenadeHandle_; break;
	case SoundEffect::BossDeath: handle = bossDeathHandle_; break;
	case SoundEffect::BossCooldown: handle = bossCooldownHandle_; break;
	case SoundEffect::MenuConfirm: handle = menuConfirmHandle_; break;
	case SoundEffect::MenuMove: handle = menuMoveHandle_; break;
	}
	if (handle != -1)
	{
		PlaySoundMem(handle, DX_PLAYTYPE_BACK, TRUE);
	}
}

void AudioManager::PlayBgm(int soundHandle)
{
	if (soundHandle == -1)
	{
		StopBgm();
		return;
	}
	// 再生中の同じ曲を指定された場合は、曲の先頭へ戻さない。
	if (currentBgmHandle_ == soundHandle && CheckSoundMem(soundHandle) == 1)
	{
		return;
	}
	StopBgm();
	PlaySoundMem(soundHandle, DX_PLAYTYPE_LOOP, TRUE);
	currentBgmHandle_ = soundHandle;
}
