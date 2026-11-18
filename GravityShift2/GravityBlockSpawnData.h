#pragma once

#include "Vector2.h"

// 重力ブロックを床側と天井側のどちらに配置するかを表す。
enum class GravityBlockSpawnSurface
{
	Floor,
	Ceiling,
};

// 重力ブロックの動作方法。
// Gravityは従来どおり全体の重力反転に従い、Vertical/Horizontalは独立して往復する。
enum class GravityBlockMotion
{
	Gravity,
	Vertical,
	Horizontal,
};

// ステージに配置する重力ブロック1つ分の初期データ。
struct GravityBlockSpawnData
{
	Vector2 position{};
	GravityBlockSpawnSurface surface = GravityBlockSpawnSurface::Floor;
	GravityBlockMotion motion = GravityBlockMotion::Gravity;
};
