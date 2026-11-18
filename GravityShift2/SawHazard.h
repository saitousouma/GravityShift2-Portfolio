#pragma once

#include "StageData.h"

class Player;

// ステージに固定されたノコギリを描画し、プレイヤーとの接触を調べる。
class SawHazard
{
public:
	// 画像の読み込みとアニメーション状態の初期化を行う。
	bool Load();
	void Release();
	void Reset();
	void Update();
	// ステージに登録された全てのノコギリを描画する。
	void Draw(const StageData& stage) const;
	// 接触したノコギリの中心X座標をダメージ方向として返す。
	bool CheckPlayerCollision(const StageData& stage, const Player& player,
		float& damageSourceCenterX) const;

	// 絵より少し小さな円を使い、角に触れただけではダメージにしない。
	static bool IntersectsRect(const Vector2& center, const Vector2& rectPosition,
		float rectWidth, float rectHeight)
	{
		float nearestX = center.x;
		float nearestY = center.y;
		if (nearestX < rectPosition.x) nearestX = rectPosition.x;
		if (nearestX > rectPosition.x + rectWidth)
			nearestX = rectPosition.x + rectWidth;
		if (nearestY < rectPosition.y) nearestY = rectPosition.y;
		if (nearestY > rectPosition.y + rectHeight)
			nearestY = rectPosition.y + rectHeight;
		const float dx = center.x - nearestX;
		const float dy = center.y - nearestY;
		return dx * dx + dy * dy < HitRadius * HitRadius;
	}

private:
	static constexpr int SourceSize = 32;
	static constexpr int SourceColumns = 4;
	static constexpr int FirstFrame = 5;
	static constexpr int FrameCount = 4;
	static constexpr int TicksPerFrame = 5;
	static constexpr int DrawSize = StageData::TileSize;
	static constexpr float HitRadius = 15.0f;
	int imageHandle_ = -1;
	int animationTick_ = 0;
};
