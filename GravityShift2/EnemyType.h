#pragma once

// 通常敵の種類。

enum class EnemyType
{
	Normal,    // 通常敵。
	Heavy,     // 耐久力が高い重量敵。
	Shooter,   // プレイヤーを狙う射撃敵。
	UpShooter, // 真上へループ弾を撃つ射撃敵。
	Charger,   // 発見後に直進する突進敵。
	Mortar,    // 地形に当たると左右へ分裂する弾を撃つ敵。
};
