#include "EditorHistory.h"

#include "EditorStageData.h"

void EditorHistory::Reset(const StageData& stageData)
{
	entries_.clear();
	currentIndex_ = 0;
	nextRevision_ = 1;
	entries_.push_back({ stageData, nextRevision_++ });
	savedRevision_ = entries_[currentIndex_].revision;
}

// 新しい編集が入ったら、分岐先のRedo履歴を捨てて記録する。
void EditorHistory::Commit(const StageData& stageData)
{
	if (entries_.empty())
	{
		Reset(stageData);
		return;
	}

	entries_.erase(entries_.begin() + currentIndex_ + 1, entries_.end());
	entries_.push_back({ stageData, nextRevision_++ });
	currentIndex_ = entries_.size() - 1;

	// 初期状態を含めて最大65件、直近64操作分を戻せるようにする。
	if (entries_.size() > MaxEntries)
	{
		entries_.erase(entries_.begin());
		--currentIndex_;
	}
}

bool EditorHistory::Undo(EditorStageData& editorStageData)
{
	if (entries_.empty() || currentIndex_ == 0)
	{
		return false;
	}

	--currentIndex_;
	editorStageData.LoadFrom(entries_[currentIndex_].stageData);
	return true;
}

bool EditorHistory::Redo(EditorStageData& editorStageData)
{
	if (currentIndex_ + 1 >= entries_.size())
	{
		return false;
	}

	++currentIndex_;
	editorStageData.LoadFrom(entries_[currentIndex_].stageData);
	return true;
}

void EditorHistory::MarkSaved()
{
	if (!entries_.empty())
	{
		savedRevision_ = entries_[currentIndex_].revision;
	}
}

bool EditorHistory::IsDirty() const
{
	return !entries_.empty() &&
		entries_[currentIndex_].revision != savedRevision_;
}
