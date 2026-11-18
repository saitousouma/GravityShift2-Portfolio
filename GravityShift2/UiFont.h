#pragma once

// 英数字と日本語でフォントを使い分けてUI文字を描画する。
namespace UiFont
{
	// フォントファイルの登録と、標準サイズの生成を行う。
	bool Load();
	void Release();
	// 混在する文字列を実際の使用フォントで測定する。
	int GetStringWidth(const wchar_t* text);
	int GetStringWidth(const wchar_t* text, int fontSize);

	// 文字列を標準サイズまたは指定サイズで描画する。
	int DrawString(
		int x,
		int y,
		const wchar_t* text,
		unsigned int color,
		unsigned int edgeColor = 0
	);

	int DrawStringSized(
		int x,
		int y,
		const wchar_t* text,
		unsigned int color,
		int fontSize,
		unsigned int edgeColor = 0
	);

	// 書式指定した文字列を標準サイズまたは指定サイズで描画する。
	int DrawFormatString(
		int x,
		int y,
		unsigned int color,
		const wchar_t* format,
		...
	);

	int DrawFormatStringSized(
		int x,
		int y,
		unsigned int color,
		int fontSize,
		const wchar_t* format,
		...
	);
}
