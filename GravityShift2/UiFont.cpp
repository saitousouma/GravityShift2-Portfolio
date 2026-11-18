#include "UiFont.h"

#include <Windows.h>

#include <cstdarg>
#include <map>
#include <string>

#include "DxLib.h"

namespace
{
	constexpr int DefaultFontSize = 16;
	constexpr wchar_t EnglishFontName[] = L"m6x11";
	constexpr wchar_t JapaneseFontName[] = L"Kaisotai Next UP B";

	struct FontHandles
	{
		int english = -1;
		int japanese = -1;
	};

	HANDLE englishFontResource = nullptr;
	HANDLE japaneseFontResource = nullptr;
	std::map<int, FontHandles> fontHandlesBySize;

	HANDLE AddFontFileWithFallback(
		const wchar_t* primaryPath,
		const wchar_t* fallbackPath)
	{
		// 実行ディレクトリが異なる場合に備え、二つのパスを順に試す。
		HANDLE handle = AddFontFile(primaryPath);
		if (handle == nullptr)
		{
			handle = AddFontFile(fallbackPath);
		}
		return handle;
	}

	const FontHandles& GetFontHandles(int fontSize)
	{
		// 同じサイズのハンドルは再生成せず、キャッシュから返す。
		const auto found = fontHandlesBySize.find(fontSize);
		if (found != fontHandlesBySize.end())
		{
			return found->second;
		}

		FontHandles handles{};
		handles.english = CreateFontToHandle(
			EnglishFontName, fontSize, 1, DX_FONTTYPE_NORMAL);
		handles.japanese = CreateFontToHandle(
			JapaneseFontName, fontSize, 1, DX_FONTTYPE_NORMAL);

		return fontHandlesBySize.emplace(fontSize, handles).first->second;
	}

	bool UsesJapaneseFont(wchar_t character)
	{
		// ASCII範囲外の文字は日本語フォント側へまとめる。
		return character > 0x7f;
	}

	int GetStringWidthSized(const wchar_t* text, int fontSize)
	{
		if (text == nullptr)
		{
			return 0;
		}

		const FontHandles& handles = GetFontHandles(fontSize);
		int width = 0;
		std::size_t runStart = 0;
		const std::wstring fullText(text);

		// 使用フォントが同じ連続部分ごとに幅を測り、合計する。
		while (runStart < fullText.size())
		{
			const bool useJapanese = UsesJapaneseFont(fullText[runStart]);
			std::size_t runEnd = runStart + 1;
			while (runEnd < fullText.size() &&
				UsesJapaneseFont(fullText[runEnd]) == useJapanese)
			{
				++runEnd;
			}

			const std::wstring run =
				fullText.substr(runStart, runEnd - runStart);
			const int fontHandle =
				useJapanese ? handles.japanese : handles.english;
			width += fontHandle != -1
				? GetDrawStringWidthToHandle(
					run.c_str(), static_cast<int>(run.size()), fontHandle)
				: GetDrawStringWidth(
					run.c_str(), static_cast<int>(run.size()));

			runStart = runEnd;
		}

		return width;
	}

	int DrawStringWithSize(
		int x,
		int y,
		const wchar_t* text,
		unsigned int color,
		unsigned int edgeColor,
		int fontSize)
	{
		if (text == nullptr)
		{
			return -1;
		}

		const FontHandles& handles = GetFontHandles(fontSize);
		int drawX = x;
		int result = 0;
		std::size_t runStart = 0;
		const std::wstring fullText(text);

		// 英数字と日本語の連続部分を分け、描画位置をつなげて表示する。
		while (runStart < fullText.size())
		{
			const bool useJapanese = UsesJapaneseFont(fullText[runStart]);
			std::size_t runEnd = runStart + 1;
			while (runEnd < fullText.size() &&
				UsesJapaneseFont(fullText[runEnd]) == useJapanese)
			{
				++runEnd;
			}

			const std::wstring run =
				fullText.substr(runStart, runEnd - runStart);
			const int fontHandle =
				useJapanese ? handles.japanese : handles.english;

			if (fontHandle != -1)
			{
				result = DrawStringToHandle(
					drawX, y, run.c_str(), color, fontHandle, edgeColor);
				drawX += GetDrawStringWidthToHandle(
					run.c_str(), static_cast<int>(run.size()), fontHandle);
			}
			else
			{
				result = ::DrawString(
					drawX, y, run.c_str(), color, edgeColor);
				drawX += GetDrawStringWidth(
					run.c_str(), static_cast<int>(run.size()));
			}

			runStart = runEnd;
		}

		return result;
	}

	int DrawFormattedStringSized(
		int x,
		int y,
		unsigned int color,
		int fontSize,
		const wchar_t* format,
		va_list arguments)
	{
		wchar_t text[2048]{};
		_vsnwprintf_s(
			text,
			_countof(text),
			_TRUNCATE,
			format,
			arguments
		);
		return DrawStringWithSize(x, y, text, color, 0, fontSize);
	}
}

bool UiFont::Load()
{
	// OSへ一時登録することで、ゲームに同梱したフォントを名前で生成する。
	englishFontResource = AddFontFileWithFallback(
		L"Assets/Fonts/m6x11.ttf",
		L"../../Assets/Fonts/m6x11.ttf");
	japaneseFontResource = AddFontFileWithFallback(
		L"Assets/Fonts/Kaisotai-Next-UP-B.ttf",
		L"../../Assets/Fonts/Kaisotai-Next-UP-B.ttf");

	if (englishFontResource == nullptr || japaneseFontResource == nullptr)
	{
		Release();
		return false;
	}

	const FontHandles& handles = GetFontHandles(DefaultFontSize);
	if (handles.english == -1 || handles.japanese == -1)
	{
		Release();
		return false;
	}

	return true;
}

void UiFont::Release()
{
	// サイズ別に生成した全ハンドルを解放してから、フォント登録を外す。
	for (const auto& entry : fontHandlesBySize)
	{
		if (entry.second.english != -1)
		{
			DeleteFontToHandle(entry.second.english);
		}
		if (entry.second.japanese != -1)
		{
			DeleteFontToHandle(entry.second.japanese);
		}
	}
	fontHandlesBySize.clear();

	if (englishFontResource != nullptr)
	{
		RemoveFontFile(englishFontResource);
		englishFontResource = nullptr;
	}
	if (japaneseFontResource != nullptr)
	{
		RemoveFontFile(japaneseFontResource);
		japaneseFontResource = nullptr;
	}
}

int UiFont::GetStringWidth(const wchar_t* text)
{
	return GetStringWidthSized(text, DefaultFontSize);
}

int UiFont::GetStringWidth(const wchar_t* text, int fontSize)
{
	return GetStringWidthSized(text, fontSize);
}

int UiFont::DrawString(
	int x,
	int y,
	const wchar_t* text,
	unsigned int color,
	unsigned int edgeColor)
{
	return DrawStringWithSize(
		x, y, text, color, edgeColor, DefaultFontSize);
}

int UiFont::DrawStringSized(
	int x,
	int y,
	const wchar_t* text,
	unsigned int color,
	int fontSize,
	unsigned int edgeColor)
{
	return DrawStringWithSize(
		x, y, text, color, edgeColor, fontSize);
}

int UiFont::DrawFormatString(
	int x,
	int y,
	unsigned int color,
	const wchar_t* format,
	...)
{
	va_list arguments;
	va_start(arguments, format);
	const int result = DrawFormattedStringSized(
		x, y, color, DefaultFontSize, format, arguments);
	va_end(arguments);
	return result;
}

int UiFont::DrawFormatStringSized(
	int x,
	int y,
	unsigned int color,
	int fontSize,
	const wchar_t* format,
	...)
{
	va_list arguments;
	va_start(arguments, format);
	const int result = DrawFormattedStringSized(
		x, y, color, fontSize, format, arguments);
	va_end(arguments);
	return result;
}
