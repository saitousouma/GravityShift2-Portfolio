#pragma once

#include "Vector2.h"
#include "EffectAnimationData.h"
#include <array>

// 1つ分のエフェクト再生を管理する。
class Effect
{
public:
	// 指定位置でエフェクトを再生開始する。
	void Start(
		const Vector2& position,
		const EffectAnimationData& animationData
	);

	// １フレーム分、再生時間を進める。
	void Update();

	// 現在のフレームを描画する。
	void Draw() const;

	// 再生中かどうか。
	bool IsActive() const;

private:
	// 再生位置と、使用するスプライトシートの設定。
	Vector2 position_{};
	EffectAnimationData animationData_{};

	// 現在のコマと、そのコマを表示している時間。
	int currentFrame_ = 0;
	int frameTimer_ = 0;
	bool isActive_ = false;
};
