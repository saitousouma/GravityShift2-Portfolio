#include "Effect.h"
#include "DxLib.h"

void Effect::Start(const Vector2& position, const EffectAnimationData& animationData)
{
	position_ = position;
	animationData_ = animationData;

	currentFrame_ = 0;
	frameTimer_ = 0;
	isActive_ = true;
}

void Effect::Update()
{
	if (!isActive_)
	{
		return;
	}

	// 指定された表示時間ごとに、次のアニメーションコマへ進める。
	frameTimer_++;

	if (frameTimer_ >= animationData_.frameDuration)
	{
		frameTimer_ = 0;
		currentFrame_++;

		if (currentFrame_ >= animationData_.frameCount)
		{
			isActive_ = false;
		}
	}
}

void Effect::Draw() const
{
	if (!isActive_ || animationData_.imageHandle == -1)
	{
		return;
	}

	const int drawX = static_cast<int>(position_.x);
	const int drawY = static_cast<int>(position_.y);

	// 現在のコマ番号から、スプライトシート内の切り出し位置を求める。
	const int sourceFrame = animationData_.startFrame + currentFrame_;
	int sourceX = sourceFrame * animationData_.frameWidth;
	int sourceY = 0;

	if (animationData_.framesPerRow > 0)
	{
		sourceX =
			(sourceFrame % animationData_.framesPerRow) * animationData_.frameWidth;
		sourceY =
			(sourceFrame / animationData_.framesPerRow) * animationData_.frameHeight;
	}

	const int drawWidth =
		static_cast<int>(animationData_.frameWidth * animationData_.drawScaleX);
	const int drawHeight =
		static_cast<int>(animationData_.frameHeight * animationData_.drawScaleY);

	// 天井側に出すエフェクトは、四隅を入れ替えて上下反転する。
	if (animationData_.flipY)
	{
		DrawRectModiGraph(
			drawX, drawY + drawHeight,
			drawX + drawWidth, drawY + drawHeight,
			drawX + drawWidth, drawY,
			drawX, drawY,
			sourceX, sourceY,
			animationData_.frameWidth,
			animationData_.frameHeight,
			animationData_.imageHandle,
			TRUE);
		return;
	}

	DrawRectExtendGraph(
		drawX, drawY, drawX + drawWidth, drawY + drawHeight,
		sourceX, sourceY,
		animationData_.frameWidth, animationData_.frameHeight,
		animationData_.imageHandle, TRUE);
}

bool Effect::IsActive() const
{
	return isActive_;
}
