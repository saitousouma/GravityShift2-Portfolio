#pragma once

// ステージごとに設定するクリア条件。
enum class ClearConditionType
{
	ReachGoal,      // ゴール地点に到達したらクリア。
	DefeatMidBoss,  // 中ボスを倒したらクリア。
	DefeatBoss      // 大ボスを倒したらクリア。
};
