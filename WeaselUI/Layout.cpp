#include "stdafx.h"
#include "Layout.h"
#include <string>

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

HRESULT DirectWriteResources::InitResources(wstring label_font_face, int label_font_point,
	wstring font_face, int font_point,
	wstring comment_font_face, int comment_font_point, UIStyle::LayoutAlignType alignType) 
{
	// prepare d2d1 resources
	DWRITE_PARAGRAPH_ALIGNMENT paragraphAliment;
	if (alignType == UIStyle::ALIGN_BOTTOM)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
	else if(alignType == UIStyle::ALIGN_CENTER)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	else
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

	HRESULT hResult = S_OK;
	vector<wstring> fontFaceStrVector;
	// text font text format set up
	split(fontFaceStrVector, font_face, is_any_of(L","));
	wstring mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// label font text format set up
	split(fontFaceStrVector, label_font_face, is_any_of(L","));
	mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(paragraphAliment);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// comment font text format set up
	split(fontFaceStrVector, comment_font_face, is_any_of(L","));
	mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(paragraphAliment);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());
	return hResult;
}

HRESULT DirectWriteResources::InitResources(const UIStyle style)
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	DWRITE_PARAGRAPH_ALIGNMENT paragraphAliment;
	if (style.align_type == UIStyle::ALIGN_BOTTOM)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
	else if(style.align_type == UIStyle::ALIGN_CENTER)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	else
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

	vector<wstring> fontFaceStrVector;
	// text font text format set up
	split(fontFaceStrVector, style.font_face, is_any_of(L","));
	wstring mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		// candidate text always center vertical
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// label font text format set up
	split(fontFaceStrVector, style.label_font_face, is_any_of(L","));
	mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(paragraphAliment);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// comment font text format set up
	split(fontFaceStrVector, style.comment_font_face, is_any_of(L","));
	mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(paragraphAliment);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	return hResult;
}

HRESULT weasel::DirectWriteResources::_SetTextFormat(IDWriteTextFormat1* _pTextFormat, const wstring fontFace, const UINT32 fontPoint, const UIStyle::LayoutAlignType alignType)
{
	HRESULT hResult = S_OK;

	DWRITE_PARAGRAPH_ALIGNMENT paragraphAliment;
	if (alignType == UIStyle::ALIGN_BOTTOM)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
	else if(alignType == UIStyle::ALIGN_CENTER)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	else
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	vector<wstring> fontFaceStrVector; 
	split(fontFaceStrVector, fontFace, is_any_of(L","));
	wstring mainFontFace = fontFaceStrVector[0];
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			fontPoint * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&_pTextFormat));
	if( _pTextFormat != NULL)
	{
		hResult = _pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		hResult = _pTextFormat->SetParagraphAlignment(paragraphAliment);
		hResult = _pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(_pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());
	return hResult;
}

void DirectWriteResources::_SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector)
{
	IDWriteFontFallback* pSysFallback;
	pDWFactory->GetSystemFontFallback(&pSysFallback);
	IDWriteFontFallback* pFontFallback = NULL;
	IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
	pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
	vector<wstring> fallbackFontsVector;
	for (UINT32 i = 1; i < fontVector.size(); i++)
	{
		split(fallbackFontsVector, fontVector[i], is_any_of(L":"));
		wstring _fontFaceWstr, firstWstr, lastWstr;
		if (fallbackFontsVector.size() == 3)
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			lastWstr = fallbackFontsVector[2];
			if (lastWstr.empty())
				lastWstr = L"10ffff";
			if (firstWstr.empty())
				firstWstr = L"0";
		}
		else if (fallbackFontsVector.size() == 2)	// fontName : codepoint
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			if (firstWstr.empty())
				firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		else if (fallbackFontsVector.size() == 1)	// if only font defined, use all range
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		UINT first = 0, last = 0x10ffff;
		try {
			first = stoi(firstWstr.c_str(), 0, 16);
		}
		catch(...){
			first = 0;
		}
		try {
			last = stoi(lastWstr.c_str(), 0, 16);
		}
		catch(...){
			first = 0x10ffff;
		}
		AddAMapping(pFontFallbackBuilder, _fontFaceWstr.c_str(), first, last);
		fallbackFontsVector.swap(vector<wstring>());
	}
	pFontFallbackBuilder->AddMappings(pSysFallback);
	pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
	pTextFormat->SetFontFallback(pFontFallback);
	SafeRelease(&pFontFallback);
	SafeRelease(&pSysFallback);
	SafeRelease(&pFontFallbackBuilder);
}

GDIFonts::GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint) 
{
	vector<wstring> fontFaceStrVector;
	split(fontFaceStrVector, labelFontFace, is_any_of(L","));
	_LabelFontFace		= fontFaceStrVector[0];
	_LabelFontPoint		= labelFontPoint;
	fontFaceStrVector.swap(vector<wstring>());

	split(fontFaceStrVector, textFontFace, is_any_of(L","));
	_TextFontFace		= fontFaceStrVector[0];
	_TextFontPoint		= textFontPoint;
	fontFaceStrVector.swap(vector<wstring>());

	split(fontFaceStrVector, commentFontFace, is_any_of(L","));
	_CommentFontFace	= fontFaceStrVector[0];
	_CommentFontPoint	= commentFontPoint;
	fontFaceStrVector.swap(vector<wstring>());
}
