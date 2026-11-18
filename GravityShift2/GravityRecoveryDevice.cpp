#include "GravityRecoveryDevice.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "Player.h"
#include "StageData.h"

bool GravityRecoveryDevice::Load()
{
	imageHandle_ = LoadGraph(L"Assets/Images/Items/health-pickup.png");
	if (imageHandle_ == -1)
	{
		imageHandle_ =
			LoadGraph(L"../../Assets/Images/Items/health-pickup.png");
	}
	return true;
}

void GravityRecoveryDevice::Release()
{
	if (imageHandle_ != -1)
	{
		DeleteGraph(imageHandle_);
		imageHandle_ = -1;
	}
}

void GravityRecoveryDevice::Initialize(const StageData& stageData)
{
	// リトライやステージ切り替えで、前回の起動状態を持ち越さない。
	devices_.fill(Device{});
	activeDeviceIndex_ = -1;
	nextDeviceIndex_ = 0;
	activeTimer_ = 0;
	switchTimer_ = 0;
	animationTimer_ = 0;
	hasDevices_ = false;
	wasEncounterActive_ = false;

	if (!stageData.hasMidBossSpawn && !stageData.hasBossSpawn)
	{
		return;
	}
	// ボスが所属するカメラグループを、回復装置を置く戦闘範囲とする。
	const int bossCameraGroupId = stageData.hasMidBossSpawn
		? stageData.midBossSpawn.cameraGroupId
		: stageData.bossSpawn.cameraGroupId;

	int areaIndex = -1;
	for (int i = 0; i < stageData.cameraAreaCount; ++i)
	{
		const int groupId = stageData.cameraAreas[i].groupId >= 0
			? stageData.cameraAreas[i].groupId : i;
		if (groupId == bossCameraGroupId)
		{
			areaIndex = i;
			break;
		}
	}
	if (areaIndex < 0)
	{
		return;
	}

	// 床側と天井側が離れるよう、範囲幅の30%と70%へ配置する。
	const CameraArea bounds = stageData.GetCameraAreaGroupBounds(areaIndex);
	const float areaWidth = static_cast<float>(bounds.right - bounds.left);
	const float floorCenterX = bounds.left + areaWidth * 0.30f;
	const float ceilingCenterX = bounds.left + areaWidth * 0.70f;
	const bool placedFloor = PlaceDevice(devices_[0], stageData,
		floorCenterX, GravityDirection::Down, bounds.top, bounds.bottom);
	const bool placedCeiling = PlaceDevice(devices_[1], stageData,
		ceilingCenterX, GravityDirection::Up, bounds.top, bounds.bottom);
	hasDevices_ = placedFloor || placedCeiling;
}

bool GravityRecoveryDevice::PlaceDevice(Device& device,
	const StageData& stageData, float centerX, GravityDirection surface,
	int areaTop, int areaBottom)
{
	// 戦闘範囲の中央から、指定された重力方向へ支持地形を探す。
	const int column = (std::max)(0, (std::min)(StageData::TileColumns - 1,
		static_cast<int>(centerX / StageData::TileSize)));
	const int firstRow = 0;
	const int lastRow = stageData.tileRowCount - 1;
	const int areaMiddleY = areaTop + (areaBottom - areaTop) / 2;
	const int middleRow = (std::max)(firstRow, (std::min)(lastRow,
		areaMiddleY / StageData::TileSize));
	int supportRow = -1;

	if (surface == GravityDirection::Down)
	{
		for (int row = middleRow; row <= lastRow; ++row)
		{
			if (StageData::IsSolidTile(stageData.tiles[row][column]))
			{
				supportRow = row;
				break;
			}
		}
	}
	else
	{
		for (int row = middleRow; row >= firstRow; --row)
		{
			if (StageData::IsSolidTile(stageData.tiles[row][column]))
			{
				supportRow = row;
				break;
			}
		}
	}

	if (supportRow < 0)
	{
		return false;
	}

	device.position.x = centerX - Width * 0.5f;
	device.position.y = surface == GravityDirection::Down
		? supportRow * StageData::TileSize - Height
		: (supportRow + 1) * StageData::TileSize;
	device.surface = surface;
	device.isPlaced = true;
	return true;
}

void GravityRecoveryDevice::Update(Player& player,
	bool isBossEncounterActive)
{
	if (!hasDevices_)
	{
		return;
	}
	++animationTimer_;

	// 戦闘外では全装置を停止し、次の戦闘開始時に初期化する。
	if (!isBossEncounterActive)
	{
		Deactivate();
		wasEncounterActive_ = false;
		return;
	}
	if (!wasEncounterActive_)
	{
		wasEncounterActive_ = true;
		nextDeviceIndex_ = 0;
		ActivateNextDevice();
	}

	// 回復直後は待機時間を置いてから、反対側の装置を起動する。
	if (activeDeviceIndex_ < 0)
	{
		if (switchTimer_ > 0)
		{
			--switchTimer_;
		}
		if (switchTimer_ <= 0)
		{
			ActivateNextDevice();
		}
		return;
	}

	Device& activeDevice = devices_[activeDeviceIndex_];
	if (IsTouchingPlayer(activeDevice, player) && player.Heal())
	{
		nextDeviceIndex_ = (activeDeviceIndex_ + 1) %
			static_cast<int>(devices_.size());
		activeDeviceIndex_ = -1;
		activeTimer_ = 0;
		switchTimer_ = SwitchDelayDuration;
		return;
	}

	// 使われないまま時間切れになった場合も、反対側へ切り替える。
	if (--activeTimer_ <= 0)
	{
		nextDeviceIndex_ = (activeDeviceIndex_ + 1) %
			static_cast<int>(devices_.size());
		activeDeviceIndex_ = -1;
		ActivateNextDevice();
	}
}

void GravityRecoveryDevice::Deactivate()
{
	activeDeviceIndex_ = -1;
	activeTimer_ = 0;
	switchTimer_ = 0;
}

void GravityRecoveryDevice::ActivateNextDevice()
{
	// 片側を配置できなかったステージでも、存在する装置だけで動作させる。
	activeDeviceIndex_ = -1;
	for (int offset = 0; offset < static_cast<int>(devices_.size()); ++offset)
	{
		const int index = (nextDeviceIndex_ + offset) %
			static_cast<int>(devices_.size());
		const Device& device = devices_[index];
		if (!device.isPlaced)
		{
			continue;
		}
		activeDeviceIndex_ = index;
		nextDeviceIndex_ = (index + 1) % static_cast<int>(devices_.size());
		activeTimer_ = ActiveDuration;
		switchTimer_ = 0;
		return;
	}
}

bool GravityRecoveryDevice::IsTouchingPlayer(
	const Device& device, const Player& player) const
{
	const float playerLeft = player.GetPosition().x;
	const float playerTop = player.GetPosition().y;
	const float playerRight = playerLeft + player.GetWidth();
	const float playerBottom = playerTop + player.GetHeight();
	return device.position.x < playerRight &&
		device.position.x + Width > playerLeft &&
		device.position.y < playerBottom &&
		device.position.y + Height > playerTop;
}

void GravityRecoveryDevice::Draw() const
{
	if (!hasDevices_)
	{
		return;
	}

	// 起動中は発光させ、停止中は暗くして利用可能な装置を示す。
	for (int i = 0; i < static_cast<int>(devices_.size()); ++i)
	{
		const Device& device = devices_[i];
		if (!device.isPlaced)
		{
			continue;
		}
		const bool isActive = i == activeDeviceIndex_;
		const int left = static_cast<int>(device.position.x);
		const int top = static_cast<int>(device.position.y);
		const int right = left + static_cast<int>(Width);
		const int bottom = top + static_cast<int>(Height);

		if (imageHandle_ != -1)
		{
			if (isActive)
			{
				const int pulse = 5 + (animationTimer_ / 8) % 3;
				SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
				SetDrawBright(120, 255, 180);
				DrawExtendGraph(left - pulse, top - pulse,
					right + pulse, bottom + pulse, imageHandle_, TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
				SetDrawBright(255, 255, 255);
			}
			else
			{
				SetDrawBright(65, 65, 75);
			}
			DrawExtendGraph(left, top, right, bottom, imageHandle_, TRUE);
			SetDrawBright(255, 255, 255);
		}
		else
		{
			DrawCircle((left + right) / 2, (top + bottom) / 2, 16,
				isActive ? GetColor(100, 255, 150) : GetColor(55, 65, 60), TRUE);
		}
	}
}
