#pragma once

#include "Vector2.h"

// ステージ上に置くノコギリ一個分の設定。
struct SawSpawnData
{
	Vector2 position{}; // ノコギリ画像の中心座標。
	bool flipX = false; // 画像を左右反転するか。
	bool flipY = false; // 画像を上下反転するか。
};
