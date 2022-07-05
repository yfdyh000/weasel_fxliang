#include "stdafx.h"
#include "Layout.h"
#include <string>
#include <fstream>

using namespace weasel;

static void GetUnicodeRange(IDWriteTextFormat* pTextFormat, IDWriteFontCollection* collection, WCHAR* fontName, DWRITE_UNICODE_RANGE* rngs, UINT32* actualRngCount)
{
	// offset calc start
	WCHAR name[64] = { 0 };
	UINT32 findex;
	BOOL exists;
	collection->FindFamilyName(name, &findex, &exists);
	IDWriteFontFamily* ffamily;
	collection->GetFontFamily(findex, &ffamily);
	IDWriteFont1* font;
	ffamily->GetFirstMatchingFont(pTextFormat->GetFontWeight(), pTextFormat->GetFontStretch(), pTextFormat->GetFontStyle(), reinterpret_cast<IDWriteFont**>(&font));
	font->GetUnicodeRanges(0xffff, rngs, actualRngCount);
	ffamily->Release();
	//collection->Release();
	font->Release();
}
static std::vector<std::wstring> ws_split(const std::wstring& in, const std::wstring& delim)
{
	std::wregex re{ delim };
	return std::vector<std::wstring> {
		std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
			std::wsregex_token_iterator()
	};
}
Layout::Layout(const UIStyle& style, const Context& context, const Status& status)
	: _style(style), _context(context), _status(status)
{
}
DirectWriteResources::DirectWriteResources() :
	dpiScaleX_(0),
	dpiScaleY_(0),
	pD2d1Factory(NULL),
	pDWFactory(NULL),
	pRenderTarget(NULL),
	pTextFormat(NULL),
	pLabelTextFormat(NULL),
	pCommentTextFormat(NULL)
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	// create factory
	if (pD2d1Factory == NULL)
		hResult = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);
	// create IDWriteFactory
	if (pDWFactory == NULL)
		hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
	/* ID2D1HwndRenderTarget */
	if (pRenderTarget == NULL)
	{
		const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, format);
		pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget);
	}
	//get the dpi information
	pD2d1Factory->GetDesktopDpi(&dpiScaleX_, &dpiScaleY_);
	dpiScaleX_ /= 72.0f;
	dpiScaleY_ /= 72.0f;
}

DirectWriteResources::~DirectWriteResources()
{
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pDWFactory);
	SafeRelease(&pD2d1Factory);
}

static void AddAMapping(IDWriteFontFallbackBuilder* pFontFallbackBuilder, const wchar_t* fname, const unsigned int start, const unsigned int end)
{
	DWRITE_UNICODE_RANGE rng;
	rng.first = start;
	rng.last = end;
	pFontFallbackBuilder->AddMapping(&rng, 1, &fname, 1);
}
#if 0
// trim from start
inline std::wstring ltrim(std::wstring &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
inline std::wstring rtrim(std::wstring &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
inline std::wstring trim(std::wstring &s)
{
    return ltrim(rtrim(s));
}
#endif

HRESULT DirectWriteResources::InitResources(std::wstring label_font_face, int label_font_point,
	std::wstring font_face, int font_point,
	std::wstring comment_font_face, int comment_font_point) 
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	std::vector<std::wstring> parsedStrV = ws_split(font_face, L",");
	hResult = pDWFactory->CreateTextFormat(parsedStrV[0].c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (parsedStrV.size() > 1)
		{
            IDWriteFontFallback* pFontFallback = NULL;
            IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
            pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
			std::vector<std::wstring> fbFontsV;
			for (int i = 1; i < parsedStrV.size(); i++)
			{
				fbFontsV = ws_split(parsedStrV[i], L":");
				// must be format like bellow, first and last must be hex number like 1f600, 1f6ff and so on, no space befor and after.
				// font name:first:last
				if (fbFontsV.size() == 3)
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), std::stoi(fbFontsV[1].c_str(), 0, 16), std::stoi(fbFontsV[2].c_str(), 0, 16));
				else if (fbFontsV.size() == 1)	// if only font defined, use all range
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), 0, 0xffffffff);
				fbFontsV.swap(std::vector<std::wstring>());
			}
			pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
			pTextFormat->SetFontFallback(pFontFallback);
			SafeRelease(&pFontFallback);
			SafeRelease(&pFontFallbackBuilder);
		}
	}
	parsedStrV.swap(std::vector<std::wstring>());
	parsedStrV = ws_split(label_font_face, L",");
	hResult = pDWFactory->CreateTextFormat(parsedStrV[0].c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (parsedStrV.size() > 1)
		{
            IDWriteFontFallback* pFontFallback = NULL;
            IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
            pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
			std::vector<std::wstring> fbFontsV;
			for (int i = 1; i < parsedStrV.size(); i++)
			{
				fbFontsV = ws_split(parsedStrV[i], L":");
				// must be format like bellow, first and last must be hex number like 1f600, 1f6ff and so on, no space befor and after.
				// font name:first:last
				if(fbFontsV.size() == 3)
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), std::stoi(fbFontsV[1].c_str(), 0, 16), std::stoi(fbFontsV[2].c_str(), 0, 16));
				else if (fbFontsV.size() == 1)	// if only font defined, use all range
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), 0, 0xffffffff);
				fbFontsV.swap(std::vector<std::wstring>());
			}
			pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
			pLabelTextFormat->SetFontFallback(pFontFallback);
			SafeRelease(&pFontFallback);
			SafeRelease(&pFontFallbackBuilder);
		}
	}
	parsedStrV.swap(std::vector<std::wstring>());
	parsedStrV = ws_split(comment_font_face, L",");
	hResult = pDWFactory->CreateTextFormat(parsedStrV[0].c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (parsedStrV.size() > 1)
		{
            IDWriteFontFallback* pFontFallback = NULL;
            IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
            pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
			std::vector<std::wstring> fbFontsV;
			for (int i = 1; i < parsedStrV.size(); i++)
			{
				fbFontsV = ws_split(parsedStrV[i], L":");
				// must be format like bellow, first and last must be hex number like 1f600, 1f6ff and so on, no space befor and after.
				// font name:first:last
				if(fbFontsV.size() == 3)
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), std::stoi(fbFontsV[1].c_str(), 0, 16), std::stoi(fbFontsV[2].c_str(), 0, 16));
				else if (fbFontsV.size() == 1)	// if only font defined, use all range
					AddAMapping(pFontFallbackBuilder, fbFontsV[0].c_str(), 0, 0xffffffff);
				fbFontsV.swap(std::vector<std::wstring>());
			}
			pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
			pCommentTextFormat->SetFontFallback(pFontFallback);
			SafeRelease(&pFontFallback);
			SafeRelease(&pFontFallbackBuilder);
		}
	}
	parsedStrV.swap(std::vector<std::wstring>());
	return hResult;
}

GDIFonts::GDIFonts(std::wstring labelFontFace, int labelFontPoint, std::wstring textFontFace, int textFontPoint, std::wstring commentFontFace, int commentFontPoint) 
{
	std::vector<std::wstring> parsedStrV = ws_split(labelFontFace, L",");
	_LabelFontFace		= parsedStrV[0];
	_LabelFontPoint		= labelFontPoint;
	parsedStrV = ws_split(textFontFace, L",");
	_TextFontFace		= parsedStrV[0];
	_TextFontPoint		= textFontPoint;
	parsedStrV = ws_split(commentFontFace, L",");
	_CommentFontFace	= parsedStrV[0];
	_CommentFontPoint	= commentFontPoint;
}
