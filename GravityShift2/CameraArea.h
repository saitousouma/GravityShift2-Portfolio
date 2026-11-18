#pragma once

// エリア内でカメラがプレイヤーを追従する方法を表す。
enum class CameraAreaMode
{
	Horizontal,
	Vertical,
	Free,
	Locked
};

// ワールド座標上のカメラ制御範囲。
// rightとbottomは範囲に含めない。
struct CameraArea
{
	CameraAreaMode mode = CameraAreaMode::Free;
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
	// 0以上の同じIDを持つ矩形を、1つのカメラ範囲として扱う。
	int groupId = -1;

	// 指定したワールド座標がエリア内ならtrueを返す。
	bool Contains(float x, float y) const
	{
		return x >= static_cast<float>(left) &&
			x < static_cast<float>(right) &&
			y >= static_cast<float>(top) &&
			y < static_cast<float>(bottom);
	}

	// オブジェクトがエリアの上端または下端をまたいでいる間も、エリアを有効にする。
	bool OverlapsVerticalSpan(float x, float spanTop, float spanBottom) const
	{
		return x >= static_cast<float>(left) &&
			x < static_cast<float>(right) &&
			spanBottom > static_cast<float>(top) &&
			spanTop < static_cast<float>(bottom);
	}

	// カメラ表示範囲をエリアの上端と下端の間に収める。
	// 画面より低いエリアでは、エリア中央にカメラを固定する。
	int ClampViewTopY(
		int desiredTop,
		int viewportHeight,
		int fixedAreaTolerance = 0) const
	{
		const int areaHeight = bottom - top;
		// 画面よりわずかに高いタイル単位のエリアで、
		// カメラが細かくずれるのを防ぐ。
		if (areaHeight <= viewportHeight + fixedAreaTolerance)
		{
			return top - (viewportHeight - areaHeight) / 2;
		}

		const int maximumTop = bottom - viewportHeight;
		if (desiredTop < top)
		{
			return top;
		}
		if (desiredTop > maximumTop)
		{
			return maximumTop;
		}
		return desiredTop;
	}
};
