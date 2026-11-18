#pragma once


// ゲーム全体の状態。
enum class GameState
{
	Title,       // タイトル画面。
	StageEditor, // ステージエディター。
	Playing,     // ステージをプレイ中。
	Pause,       // ポーズメニュー。
	GameClear,   // ステージクリア画面。
	GameOver     // ゲームオーバー画面。
};
