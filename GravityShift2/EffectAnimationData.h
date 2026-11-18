#pragma once


// エフェクトの種類ごとに使用するアニメーション設定。
struct EffectAnimationData
{
	int imageHandle = -1;

	// スプライトシート内の再生コマ数。
	int frameCount = 1;

	// 1コマの横幅。
	int frameWidth = 64;

	// 1コマの縦幅。
	int frameHeight = 64;

	// 0なら、全コマが横一列に並んでいるものとして扱う。
	int framesPerRow = 0;

	// 何フレームごとに次のコマへ進むか。
	// 値が小さいほど速く再生する。
	int frameDuration = 4;

	// スプライトシートの何コマ目から再生するか。
	int startFrame = 0;

	// 描画時だけ横方向へどれだけ拡大するか。
	float drawScaleX = 1.0f;

	// 描画時だけ縦方向へどれだけ拡大するか。
	float drawScaleY = 1.0f;

	// 天井側の足元から出すエフェクトは上下を反転して描く。
	bool flipY = false;
};
