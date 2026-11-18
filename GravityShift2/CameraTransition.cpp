#include "CameraTransition.h"

#include <cmath>

namespace
{
	// 縦移動用・固定カメラでは、カメラのX座標が範囲側に拘束される。
	bool FixesX(CameraAreaMode mode)
	{
		return mode == CameraAreaMode::Vertical ||
			mode == CameraAreaMode::Locked;
	}

	// 横移動用・固定カメラでは、カメラのY座標が範囲側に拘束される。
	bool FixesY(CameraAreaMode mode)
	{
		return mode == CameraAreaMode::Horizontal ||
			mode == CameraAreaMode::Locked;
	}
}

void CameraTransition::Reset()
{
	initialized_ = false;
	blendX_ = false;
	blendY_ = false;
	elapsedFrames_ = 0;
	activeAreaIndex_ = -1;
	activeMode_ = CameraAreaMode::Free;
}

void CameraTransition::Update(
	int targetX, int targetY, int areaIndex, CameraAreaMode mode)
{
	if (!initialized_)
	{
		currentX_ = targetX;
		currentY_ = targetY;
		activeAreaIndex_ = areaIndex;
		activeMode_ = mode;
		initialized_ = true;
		return;
	}

	if (areaIndex != activeAreaIndex_ || mode != activeMode_)
	{
		// 補間中に境界をまたぎ直しても、現在の表示位置から再出発する。
		blendX_ = blendX_ || FixesX(activeMode_) || FixesX(mode);
		blendY_ = blendY_ || FixesY(activeMode_) || FixesY(mode);
		startX_ = currentX_;
		startY_ = currentY_;
		elapsedFrames_ = 0;
		activeAreaIndex_ = areaIndex;
		activeMode_ = mode;
	}

	if (elapsedFrames_ < DurationFrames && (blendX_ || blendY_))
	{
		// 始点と目標の両端で減速する補間にして、切り替え感を抑える。
		++elapsedFrames_;
		const float progress =
			static_cast<float>(elapsedFrames_) / DurationFrames;
		const float eased = progress * progress * (3.0f - 2.0f * progress);
		currentX_ = blendX_
			? static_cast<int>(std::lround(startX_ + (targetX - startX_) * eased))
			: targetX;
		currentY_ = blendY_
			? static_cast<int>(std::lround(startY_ + (targetY - startY_) * eased))
			: targetY;
		if (elapsedFrames_ == DurationFrames)
		{
			blendX_ = false;
			blendY_ = false;
		}
		return;
	}

	currentX_ = targetX;
	currentY_ = targetY;
}
