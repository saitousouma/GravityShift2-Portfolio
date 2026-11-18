#include "ScoreManager.h"
#include "UiFont.h"
#include "DxLib.h"

void ScoreManager::Reset()
{
	score_ = 0;

	for (ScorePopup& popup : popups_)
	{
		popup = ScorePopup{};
	}
}

void ScoreManager::AddScore(
	int score,
	const Vector2& popupPosition,
	int fontSize,
	int color
)
{
	score_ += score;

	// 色が省略された場合は、スコア用の黄色を使用する。
	const int popupColor =
		color == -1
		? GetColor(255, 230, 80)
		: color;

	// 非表示中の枠を再利用し、動的なメモリ確保を行わない。
	for (ScorePopup& popup : popups_)
	{
		if (!popup.IsActive())
		{
			popup.Start(popupPosition, score, fontSize, popupColor);
			return;
		}
	}
}

void ScoreManager::Update()
{
	for (ScorePopup& popup : popups_)
	{
		popup.Update();
	}
}

void ScoreManager::Draw() const
{
	DrawTotalScore();
	DrawPopups();
}

void ScoreManager::DrawPopups() const
{
	for (const ScorePopup& popup : popups_)
	{
		popup.Draw();
	}
}

void ScoreManager::DrawTotalScore() const
{
	UiFont::DrawFormatString(
		1000,
		40,
		GetColor(255, 255, 255),
		L"Score: %d",
		score_
	);
}

int ScoreManager::GetScore() const
{
	return score_;
}


