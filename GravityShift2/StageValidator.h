#pragma once

#include <vector>

#include "StageData.h"

// 検証結果の位置が-1なら、ステージ全体に関する指摘を表す。
struct StageValidationIssue
{
	const wchar_t* message = L"";
	int column = -1;
	int row = -1;
};

// ステージの編集内容を保存前に点検する。元データは変更しない。
class StageValidator
{
public:
	static std::vector<StageValidationIssue> Validate(const StageData& stageData);
};
