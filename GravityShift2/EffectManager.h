#pragma once

#include "Effect.h"
#include "Vector2.h"
#include "EffectAnimationData.h"
#include <array>


// 複数のエフェクトをまとめて生成、更新、描画するクラス。
class EffectManager
{
public:

	// 使用するエフェクト画像とアニメーション設定を読み込む。
	bool Load();
	// ステージ切り替え時に再生中のエフェクトを全て止める。
	void Clear();

	// 指定位置で敵撃破エフェクトを再生する。
	void PlayEnemyDestroyEffect(const Vector2& position);

	// ボス撃破位置にエフェクトを出す。
	void PlayBossDestroyEffect(const Vector2& position);

	// プレイヤー死亡位置にエフェクトを出す。
	void PlayPlayerDestroyEffect(const Vector2& position);

	// レーザー発射位置にエフェクトを出す。
	void PlayLaserFireEffect(const Vector2& position);

	// 重力ブロックが着地した位置にエフェクトを出す。
	void PlayGravityBlockLandingEffect(const Vector2& position);

	// チェックポイントの起動位置にエフェクトを出す。
	void PlayCheckpointEffect(const Vector2& position);

	// 重力反転前の足元からエフェクトを出す。
	void PlayGravityReverseEffect(const Vector2& playerPosition,
		float playerWidth, float playerHeight, bool isFootAtTop);

	// 有効な全エフェクトを更新する。
	void Update();

	// 有効な全エフェクトを描画する。
	void Draw() const;

private:

	// 同時再生数を固定し、エフェクト生成時の動的確保を避ける。
	static constexpr int MaxEffects = 32;

	std::array<Effect, MaxEffects> effects_{};
    
	// 種類ごとの画像ハンドルとスプライトシート設定。
	EffectAnimationData enemyDestroyAnimation_{};
	EffectAnimationData bossDestroyAnimation_{};
	EffectAnimationData playerDestroyAnimation_{};
	EffectAnimationData laserFireAnimation_{};
	EffectAnimationData gravityBlockLandingAnimation_{};
	EffectAnimationData checkpointAnimation_{};
	EffectAnimationData gravityReverseAnimation_{};

};
