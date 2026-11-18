#pragma once

// ステージ上の落下穴1つ分の矩形。
// 上下座標が未指定の古いデータは、上下両方の外周穴として扱う。
struct FallHole
{
	int left = 0;
	int right = 0;
	int top = -1;
	int bottom = -1;

	bool IsLegacyBoundaryHole() const
	{
		return top < 0 && bottom < 0;
	}

	bool OverlapsHorizontal(int targetLeft, int targetRight) const
	{
		return targetLeft < right && targetRight > left;
	}

	bool ContainsHorizontal(float targetLeft, float targetRight) const
	{
		return targetLeft >= static_cast<float>(left) &&
			targetRight <= static_cast<float>(right);
	}

	bool OverlapsTile(int tileLeft, int tileRight, int tileTop,
		int tileBottom, int stageHeight, int boundaryThickness) const
	{
		if (!OverlapsHorizontal(tileLeft, tileRight))
		{
			return false;
		}

		if (IsLegacyBoundaryHole())
		{
			return tileTop < boundaryThickness ||
				tileBottom > stageHeight - boundaryThickness;
		}

		return tileTop < bottom && tileBottom > top;
	}

	bool OpensCeiling() const
	{
		return IsLegacyBoundaryHole() || top <= 0;
	}

	bool OpensFloor(int stageHeight) const
	{
		return IsLegacyBoundaryHole() || bottom >= stageHeight;
	}
};
