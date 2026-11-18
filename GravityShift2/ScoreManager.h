#pragma once
#include <array>
#include "ScorePopup.h"
#include "Vector2.h"

// 合計スコアと、敵撃破時に表示するスコア文字を管理する。
class ScoreManager
{
public:
	// 合計値と表示中のポップアップを初期化する。
	void Reset();

	// 合計へ加算し、空いている枠にスコア表示を生成する。
	void AddScore(
		int score,
		const Vector2& popupPosition,
		int fontSize = 20,
		int color = -1
	);

	void Update();

	void Draw() const;

	// 敵を倒した場所から出るスコア表示だけを描画する。
	void DrawPopups() const;

	// 画面に固定表示する合計スコアだけを描画する。
	void DrawTotalScore() const;

	int GetScore() const;

private:
	// 同時に表示できるスコア文字の最大数。
	static constexpr int MaxPopups = 16;

	int score_ = 0;

	std::array<ScorePopup, MaxPopups> popups_{};

};
