#pragma once

#include <array>

// ゲーム内で使用するコントローラーのボタンを定義する。
// DXライブラリ固有のボタン番号は、この入力管理クラスの中だけで扱う。
enum class GamepadButton
{
	// 十字キーまたは左スティックから取得する方向入力。
	Up,
	Down,
	Left,
	Right,

	// GP20xで使用するアクションボタン。
	A,
	B,
	X,
	Y,
	RightShoulder,
	Start,
	Back,

	Count
};

class InputManager
{
public:
	// 毎フレームの先頭でキーボードとコントローラーの状態を取得する。
	void Update();

	// キーボードの入力状態を確認する。
	// 矢印キーにはコントローラーの方向入力も含める。
	bool IsPressed(int keyCode) const;
	bool IsTriggered(int keyCode) const;

	// コントローラーのボタン入力を直接確認する。
	bool IsGamepadPressed(GamepadButton button) const;
	bool IsGamepadTriggered(GamepadButton button) const;

	// キーボードとコントローラーに共通するゲーム内アクション。
	bool IsConfirmTriggered() const;
	bool IsCancelTriggered() const;
	bool IsJumpTriggered() const;
	bool IsShootTriggered() const;
	bool IsLaserTriggered() const;
	bool IsGravityTriggered() const;
	bool IsDashTriggered() const;
	bool IsPauseTriggered() const;

private:
	static constexpr int KeyCount = 256;
	static constexpr int GamepadButtonCount =
		static_cast<int>(GamepadButton::Count);

	// 押した瞬間を判定するため、現在と1フレーム前の状態を保存する。
	std::array<char, KeyCount> currentKeys_{};
	std::array<char, KeyCount> previousKeys_{};

	std::array<char, GamepadButtonCount> currentGamepadButtons_{};
	std::array<char, GamepadButtonCount> previousGamepadButtons_{};
};
