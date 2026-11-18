#pragma once

#include "StageData.h"

// エディターで変更したステージをテキストファイルへ保存するクラス。
class StageFileWriter
{
public:
	// stageNumberに対応するstageN.txtをバックアップしてから保存する。
	static bool Save(const StageData& stageData);
};
