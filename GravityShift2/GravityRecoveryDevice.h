#pragma once

#include <array>

#include "GravitySystem.h"
#include "Vector2.h"

struct StageData;
class Player;

// ボス戦中、床側と天井側で交互に起動する回復装置を管理する。
class GravityRecoveryDevice
{
public:
	// 回復アイテム画像を読み込み、ステージ上の設置位置を決める。
	bool Load();
	void Release();
	void Initialize(const StageData& stageData);
	// ボス戦の状態に合わせて装置を切り替え、接触時に回復する。
	void Update(Player& player, bool isBossEncounterActive);
	void Deactivate();
	void Draw() const;

private:
	// 一つの装置が置かれている座標と接地面を表す。
	struct Device
	{
		Vector2 position{};
		GravityDirection surface = GravityDirection::Down;
		bool isPlaced = false;
	};

	// 指定したX座標の床または天井を探し、装置を地形へ接地させる。
	bool PlaceDevice(Device& device, const StageData& stageData,
		float centerX, GravityDirection surface,
		int areaTop, int areaBottom);
	// 設置できた装置の中から、次の一台を起動する。
	void ActivateNextDevice();
	bool IsTouchingPlayer(const Device& device, const Player& player) const;

	std::array<Device, 2> devices_{};
	int imageHandle_ = -1;
	int activeDeviceIndex_ = -1;
	int nextDeviceIndex_ = 0;
	int activeTimer_ = 0;
	int switchTimer_ = 0;
	int animationTimer_ = 0;
	bool hasDevices_ = false;
	bool wasEncounterActive_ = false;

	static constexpr float Width = 32.0f;
	static constexpr float Height = 34.0f;
	static constexpr int ActiveDuration = 60 * 8;
	static constexpr int SwitchDelayDuration = 60 * 3;
};
