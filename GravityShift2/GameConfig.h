#pragma once

// 画面サイズなど、ゲーム全体で共有する設定値を管理する。
namespace GameConfig
{
	// ウィンドウと描画先の基本設定。
	constexpr int ScreenWidth = 1280;
	constexpr int ScreenHeight = 720;
	constexpr int ColorBitDepth = 32;

	// 画面内で床と天井として扱う基準Y座標。
	constexpr int CeilingY = 80;
	constexpr int FloorY = 640;
}
