#pragma once

#include <cstddef>
#include <vector>

#include "StageData.h"

class EditorStageData;

// 編集操作ごとのステージ状態を保持し、元に戻す・やり直すを管理する。
class EditorHistory
{
public:
	// 編集開始時の状態を履歴の基準として登録する。
	void Reset(const StageData& stageData);
	// 編集後の状態を新しい履歴として追加する。
	void Commit(const StageData& stageData);
	// 履歴を一つ戻す、または一つ進める。
	bool Undo(EditorStageData& editorStageData);
	bool Redo(EditorStageData& editorStageData);
	// 現在の履歴を保存済みとして記録する。
	void MarkSaved();
	// 保存後に編集が行われているかを返す。
	bool IsDirty() const;

private:
	struct Entry
	{
		StageData stageData;
		// 履歴の位置が変わっても保存状態を識別するための番号。
		unsigned long long revision = 0;
	};

	static constexpr std::size_t MaxEntries = 65;
	std::vector<Entry> entries_;
	std::size_t currentIndex_ = 0;
	unsigned long long nextRevision_ = 1;
	unsigned long long savedRevision_ = 0;
};
