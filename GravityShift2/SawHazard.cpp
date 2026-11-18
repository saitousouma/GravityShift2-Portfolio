#include "SawHazard.h"

#include "Player.h"
#include "DxLib.h"

bool SawHazard::Load()
{
	imageHandle_ = LoadGraph(L"Assets/Images/Hazards/saw-sheet.png");
	if (imageHandle_ == -1)
	{
		imageHandle_ = LoadGraph(L"../../Assets/Images/Hazards/saw-sheet.png");
	}
	// 画像が見つからなくても円の仮表示でゲームを続けられる。
	return true;
}

void SawHazard::Release()
{
	if (imageHandle_ != -1)
	{
		DeleteGraph(imageHandle_);
		imageHandle_ = -1;
	}
}

void SawHazard::Reset()
{
	animationTick_ = 0;
}

void SawHazard::Update()
{
	animationTick_ = (animationTick_ + 1) % (FrameCount * TicksPerFrame);
}

void SawHazard::Draw(const StageData& stage) const
{
	// 一枚のスプライトシートから現在の回転フレームを切り出す。
	const int frame = FirstFrame + animationTick_ / TicksPerFrame;
	for (int i = 0; i < stage.sawSpawnCount; ++i)
	{
		const SawSpawnData& saw = stage.sawSpawns[i];
		const Vector2& center = saw.position;
		if (imageHandle_ != -1)
		{
			DrawRectRotaGraph(static_cast<int>(center.x),
				static_cast<int>(center.y),
				(frame % SourceColumns) * SourceSize,
				(frame / SourceColumns) * SourceSize,
				SourceSize, SourceSize,
				static_cast<double>(DrawSize) / SourceSize, 0.0,
				imageHandle_, TRUE, saw.flipX, saw.flipY);
		}
		else
		{
			DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
				DrawSize / 2, GetColor(255, 120, 65), TRUE);
		}
	}
}

bool SawHazard::CheckPlayerCollision(const StageData& stage,
	const Player& player, float& damageSourceCenterX) const
{
	// 最初に接触したノコギリだけを返し、1フレームの多重ダメージを防ぐ。
	for (int i = 0; i < stage.sawSpawnCount; ++i)
	{
		if (IntersectsRect(stage.sawSpawns[i].position, player.GetPosition(),
			player.GetWidth(), player.GetHeight()))
		{
			damageSourceCenterX = stage.sawSpawns[i].position.x;
			return true;
		}
	}
	return false;
}
