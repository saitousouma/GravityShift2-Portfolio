#include "InputManager.h"
#include "DxLib.h"

namespace
{
	// スティック中央付近の小さな入力を無視するためのしきい値。
	constexpr int AnalogStickThreshold = 16000;
	constexpr int DirectInputAnalogThreshold = 500;

	// ボタンの列挙値を入力状態配列の添字へ変換する。
	int ToIndex(GamepadButton button)
	{
		return static_cast<int>(button);
	}
}

void InputManager::Update()
{
	// 新しい入力を取得する前に、現在のキー状態を前フレーム分として保存する。
	previousKeys_ = currentKeys_;
	GetHitKeyStateAll(currentKeys_.data());

	// コントローラーも前フレームの状態を保存してから、現在の状態を初期化する。
	previousGamepadButtons_ = currentGamepadButtons_;
	currentGamepadButtons_.fill(0);

	// 未接続の場合は初期化した状態を維持し、誤入力を防ぐ。
	if (GetJoypadNum() <= 0)
	{
		return;
	}

	// GP20xのボタン表記と正確に対応させるため、XInputを優先して取得する。
	XINPUT_STATE xInputState{};
	if (CheckJoypadXInput(DX_INPUT_PAD1) == TRUE &&
		GetJoypadXInputState(DX_INPUT_PAD1, &xInputState) == 0)
	{
		// 十字キーと左スティックを、同じ上下左右入力としてまとめる。
		currentGamepadButtons_[ToIndex(GamepadButton::Up)] =
			xInputState.Buttons[XINPUT_BUTTON_DPAD_UP] != 0 ||
			xInputState.ThumbLY > AnalogStickThreshold;

		currentGamepadButtons_[ToIndex(GamepadButton::Down)] =
			xInputState.Buttons[XINPUT_BUTTON_DPAD_DOWN] != 0 ||
			xInputState.ThumbLY < -AnalogStickThreshold;

		currentGamepadButtons_[ToIndex(GamepadButton::Left)] =
			xInputState.Buttons[XINPUT_BUTTON_DPAD_LEFT] != 0 ||
			xInputState.ThumbLX < -AnalogStickThreshold;

		currentGamepadButtons_[ToIndex(GamepadButton::Right)] =
			xInputState.Buttons[XINPUT_BUTTON_DPAD_RIGHT] != 0 ||
			xInputState.ThumbLX > AnalogStickThreshold;

		// XInputのボタン名に合わせて各アクションボタンを保存する。
		currentGamepadButtons_[ToIndex(GamepadButton::A)] =
			xInputState.Buttons[XINPUT_BUTTON_A] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::B)] =
			xInputState.Buttons[XINPUT_BUTTON_B] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::X)] =
			xInputState.Buttons[XINPUT_BUTTON_X] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::Y)] =
			xInputState.Buttons[XINPUT_BUTTON_Y] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::RightShoulder)] =
			xInputState.Buttons[XINPUT_BUTTON_RIGHT_SHOULDER] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::Start)] =
			xInputState.Buttons[XINPUT_BUTTON_START] != 0;

		currentGamepadButtons_[ToIndex(GamepadButton::Back)] =
			xInputState.Buttons[XINPUT_BUTTON_BACK] != 0;

		return;
	}

	// XInputで取得できない場合は、DirectInputのボタン番号を使用する。
	const int padState = GetJoypadInputState(DX_INPUT_PAD1);

	int analogX = 0;
	int analogY = 0;
	GetJoypadAnalogInput(&analogX, &analogY, DX_INPUT_PAD1);

	// DirectInputはスティックの値域が異なるため、専用のしきい値を使用する。
	currentGamepadButtons_[ToIndex(GamepadButton::Up)] =
		(padState & PAD_INPUT_UP) != 0 ||
		analogY < -DirectInputAnalogThreshold;

	currentGamepadButtons_[ToIndex(GamepadButton::Down)] =
		(padState & PAD_INPUT_DOWN) != 0 ||
		analogY > DirectInputAnalogThreshold;

	currentGamepadButtons_[ToIndex(GamepadButton::Left)] =
		(padState & PAD_INPUT_LEFT) != 0 ||
		analogX < -DirectInputAnalogThreshold;

	currentGamepadButtons_[ToIndex(GamepadButton::Right)] =
		(padState & PAD_INPUT_RIGHT) != 0 ||
		analogX > DirectInputAnalogThreshold;

	// GP20xのDirectInputモードでは、各ボタンを番号で取得する。
	currentGamepadButtons_[ToIndex(GamepadButton::A)] =
		(padState & PAD_INPUT_1) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::B)] =
		(padState & PAD_INPUT_2) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::X)] =
		(padState & PAD_INPUT_3) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::Y)] =
		(padState & PAD_INPUT_4) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::RightShoulder)] =
		(padState & PAD_INPUT_6) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::Start)] =
		(padState & PAD_INPUT_8) != 0;

	currentGamepadButtons_[ToIndex(GamepadButton::Back)] =
		(padState & PAD_INPUT_7) != 0;
}

// キー、または対応する方向入力が押されている間はtrueを返す。
bool InputManager::IsPressed(int keyCode) const
{
	// 不正なキーコードで固定長配列を参照しないようにする。
	if (keyCode < 0 || keyCode >= KeyCount)
	{
		return false;
	}

	if (currentKeys_[keyCode] != 0)
	{
		return true;
	}

	// 矢印キーとコントローラーの方向入力で、同じ移動処理を共有する。
	switch (keyCode)
	{
	case KEY_INPUT_UP:
		return IsGamepadPressed(GamepadButton::Up);
	case KEY_INPUT_DOWN:
		return IsGamepadPressed(GamepadButton::Down);
	case KEY_INPUT_LEFT:
		return IsGamepadPressed(GamepadButton::Left);
	case KEY_INPUT_RIGHT:
		return IsGamepadPressed(GamepadButton::Right);
	default:
		return false;
	}
}

// 入力を押した最初の1フレームだけtrueを返す。
bool InputManager::IsTriggered(int keyCode) const
{
	if (keyCode < 0 || keyCode >= KeyCount)
	{
		return false;
	}

	if (currentKeys_[keyCode] != 0 && previousKeys_[keyCode] == 0)
	{
		return true;
	}

	// コントローラーの方向入力にも、押した瞬間の判定を適用する。
	switch (keyCode)
	{
	case KEY_INPUT_UP:
		return IsGamepadTriggered(GamepadButton::Up);
	case KEY_INPUT_DOWN:
		return IsGamepadTriggered(GamepadButton::Down);
	case KEY_INPUT_LEFT:
		return IsGamepadTriggered(GamepadButton::Left);
	case KEY_INPUT_RIGHT:
		return IsGamepadTriggered(GamepadButton::Right);
	default:
		return false;
	}
}

// コントローラーのボタンが現在押されているか確認する。
bool InputManager::IsGamepadPressed(GamepadButton button) const
{
	const int index = ToIndex(button);

	return index >= 0 && index < GamepadButtonCount &&
		currentGamepadButtons_[index] != 0;
}

// コントローラーのボタンが、離した状態から押した状態へ変化したか確認する。
bool InputManager::IsGamepadTriggered(GamepadButton button) const
{
	const int index = ToIndex(button);

	return index >= 0 && index < GamepadButtonCount &&
		currentGamepadButtons_[index] != 0 &&
		previousGamepadButtons_[index] == 0;
}

// メニュー決定は、キーボードのEnterまたはコントローラーのAに割り当てる。
bool InputManager::IsConfirmTriggered() const
{
	return IsTriggered(KEY_INPUT_RETURN) ||
		IsGamepadTriggered(GamepadButton::A);
}

// メニューの戻る操作は、Escape、B、BACKに割り当てる。
bool InputManager::IsCancelTriggered() const
{
	return IsTriggered(KEY_INPUT_ESCAPE) ||
		IsGamepadTriggered(GamepadButton::B) ||
		IsGamepadTriggered(GamepadButton::Back);
}

// ここから下で、ゲーム中の操作をキーボードとコントローラーに割り当てる。
bool InputManager::IsJumpTriggered() const
{
	return IsTriggered(KEY_INPUT_SPACE) ||
		IsGamepadTriggered(GamepadButton::A);
}

bool InputManager::IsShootTriggered() const
{
	return IsTriggered(KEY_INPUT_Z) ||
		IsGamepadTriggered(GamepadButton::B);
}

bool InputManager::IsLaserTriggered() const
{
	return IsTriggered(KEY_INPUT_X) ||
		IsGamepadTriggered(GamepadButton::RightShoulder);
}

bool InputManager::IsGravityTriggered() const
{
	return IsTriggered(KEY_INPUT_G) ||
		IsGamepadTriggered(GamepadButton::Y);
}

bool InputManager::IsDashTriggered() const
{
	return IsTriggered(KEY_INPUT_LSHIFT) ||
		IsTriggered(KEY_INPUT_RSHIFT) ||
		IsGamepadTriggered(GamepadButton::X);
}

bool InputManager::IsPauseTriggered() const
{
	return IsTriggered(KEY_INPUT_P) ||
		IsGamepadTriggered(GamepadButton::Start);
}
