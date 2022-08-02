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
		DirectWriteResources();
		~DirectWriteResources();

		HRESULT InitResources(wstring label_font_face, int label_font_point,
			wstring font_face, int font_point,
			wstring comment_font_face, int comment_font_point, UIStyle::LayoutAlignType alignType=UIStyle::ALIGN_BOTTOM);
		HRESULT InitResources(const UIStyle style);
		float dpiScaleX_, dpiScaleY_;
		ID2D1Factory* pD2d1Factory;
		IDWriteFactory2* pDWFactory;
		ID2D1DCRenderTarget* pRenderTarget;
		IDWriteTextFormat1* pTextFormat;
		IDWriteTextFormat1* pLabelTextFormat;
		IDWriteTextFormat1* pCommentTextFormat;
	private:
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, DWRITE_FONT_WEIGHT& fontWeight);
		void _SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector);
	};

	class GDIFonts
	{
	public:
		~GDIFonts() {}
		GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint);
		void _ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, LONG& fontWeight);
		wstring _LabelFontFace;
		wstring _TextFontFace;
		wstring _CommentFontFace;
		LONG _LabelFontWeight;
		LONG _TextFontWeight;
		LONG _CommentFontWeight;
		int _LabelFontPoint;
		int _TextFontPoint;
		int _CommentFontPoint;
	};
};
