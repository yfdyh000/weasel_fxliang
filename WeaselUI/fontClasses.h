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
	private:
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle);
		void _SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector);
	};
	class FontInfo
	{
	public:
		wstring m_FontFace;
		int m_FontPoint;
		int m_FontWeight;
	};
	class GDIFonts
	{
	public:
		~GDIFonts() {}
		//GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint);
		GDIFonts(const UIStyle* style);
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, int& fontWeight);
		FontInfo m_LabelFont;
		FontInfo m_TextFont;
		FontInfo m_CommentFont;
	};
};
