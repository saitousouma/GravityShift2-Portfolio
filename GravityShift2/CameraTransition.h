#pragma once

#include "CameraArea.h"

// カメラエリア切り替え時だけ、位置が急変する軸を滑らかにつなぐ。
class CameraTransition
{
public:
	// ステージ開始時は最初の更新で目標位置へ即座に合わせる。
	void Reset();
	// カメラ範囲が変わった軸だけを補間し、現在位置を更新する。
	void Update(int targetX, int targetY, int areaIndex, CameraAreaMode mode);

	int GetX() const { return currentX_; }
	int GetY() const { return currentY_; }

private:
	// 約3分の1秒かけて範囲境界を滑らかにつなぐ。
	static constexpr int DurationFrames = 20;

	bool initialized_ = false;
	bool blendX_ = false;
	bool blendY_ = false;
	int activeAreaIndex_ = -1;
	int elapsedFrames_ = 0;
	int startX_ = 0;
	int startY_ = 0;
	int currentX_ = 0;
	int currentY_ = 0;
	CameraAreaMode activeMode_ = CameraAreaMode::Free;
};
