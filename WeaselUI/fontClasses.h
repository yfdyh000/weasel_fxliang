#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include <vector>
#include <regex>
#include <d2d1.h>
#include <dwrite_2.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace weasel;
using namespace boost::algorithm;
using namespace std;

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

namespace weasel
{
	enum FONT_WEIGHT_ENUM
	{
		FONT_WEIGHT_THIN = 0,
		FONT_WEIGHT_EXTRA_LIGHT,
		FONT_WEIGHT_ULTRA_LIGHT,
		FONT_WEIGHT_LIGHT,
		FONT_WEIGHT_SEMI_LIGHT,
		FONT_WEIGHT_MEDIUM,
		FONT_WEIGHT_DEMI_BOLD,
		FONT_WEIGHT_SEMI_BOLD,
		FONT_WEIGHT_BOLD,
		FONT_WEIGHT_EXTRA_BOLD,
		FONT_WEIGHT_ULTRA_BOLD,
		FONT_WEIGHT_BLACK,
		FONT_WEIGHT_HEAVY,
		FONT_WEIGHT_EXTRA_BLACK,
		FONT_WEIGHT_ULTRA_BLACK,
		FONT_WEIGHT_NORMAL
	};
	enum FONT_STYLE
	{
		FONT_STYLE_NORMAL = 0,
		FONT_STYLE_ITALIC,
		FONT_STYLE_OBLIQUE
	};
	class FontInfo
	{
	public:
		wstring m_FontFace;
		int m_FontPoint;
		int m_FontWeight;
		int m_FontStyle;
	};
	class DirectWriteResources
	{
	public:
		DirectWriteResources(const weasel::UIStyle& style);
		~DirectWriteResources();

		HRESULT InitResources(wstring label_font_face, int label_font_point,
			wstring font_face, int font_point,
			wstring comment_font_face, int comment_font_point);
		HRESULT InitResources(const UIStyle& style);
		float dpiScaleX_, dpiScaleY_;
		ID2D1Factory* pD2d1Factory;
		IDWriteFactory2* pDWFactory;
		ID2D1DCRenderTarget* pRenderTarget;
		IDWriteTextFormat1* pTextFormat;
		IDWriteTextFormat1* pLabelTextFormat;
		IDWriteTextFormat1* pCommentTextFormat;
		FontInfo TextFontInfo;
		FontInfo LabelTextFontInfo;
		FontInfo CommentTextFontInfo;
		bool VerifyChanged(const weasel::UIStyle& style);
	private:
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle, FontInfo& fontInfo);
		void _SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector);
	};
	class GDIFonts
	{
	public:
		~GDIFonts() {}
		//GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint);
		GDIFonts(const UIStyle& style);
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, int& fontWeight);
		FontInfo m_LabelFont;
		FontInfo m_TextFont;
		FontInfo m_CommentFont;
	};
};
