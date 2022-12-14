#pragma once

#include <WeaselCommon.h>
#include <WeaselUI.h>
#include <regex>
#include <dwrite.h>
#include <d2d1.h>

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


class DirectWriteResources
{
public:
	DirectWriteResources();
	~DirectWriteResources();

	HRESULT InitResources(std::wstring label_font_face, int label_font_point,
		std::wstring font_face, int font_point,
		std::wstring comment_font_face, int comment_font_point);

	float dpiScaleX_, dpiScaleY_;
	ID2D1Factory* pD2d1Factory;
	IDWriteFactory* pDWFactory;
	ID2D1DCRenderTarget* pRenderTarget;
	IDWriteTextFormat* pTextFormat;
	IDWriteTextFormat* pLabelTextFormat;
	IDWriteTextFormat* pCommentTextFormat;
};

class GDIFonts
{
public:
	GDIFonts() {};
	~GDIFonts(){}
	GDIFonts(std::wstring labelFontFace, int labelFontPoint, std::wstring textFontFace, int textFontPoint, std::wstring commentFontFace, int commentFontPoint) 
	{
		_LabelFontFace		= labelFontFace;
		_LabelFontPoint		= labelFontPoint;
		_TextFontFace		= textFontFace;
		_TextFontPoint		= textFontPoint;
		_CommentFontFace	= commentFontFace;
		_CommentFontPoint	= commentFontPoint;
	}
	std::wstring _LabelFontFace;
	std::wstring _TextFontFace;
	std::wstring _CommentFontFace;
	int _LabelFontPoint;
	int _TextFontPoint;
	int _CommentFontPoint;
};

namespace weasel
{
	class Layout
	{
	public:
		Layout(const UIStyle &style, const Context &context, const Status &status);

		virtual void DoLayout(CDCHandle dc, GDIFonts* pFonts = 0) = 0;
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR) = 0;
		/* All points in this class is based on the content area */
		/* The top-left corner of the content area is always (0, 0) */
		virtual CSize GetContentSize() const = 0;
		virtual CRect GetPreeditRect() const = 0;
		virtual CRect GetAuxiliaryRect() const = 0;
		virtual CRect GetHighlightRect() const = 0;
		virtual CRect GetCandidateLabelRect(int id) const = 0;
		virtual CRect GetCandidateTextRect(int id) const = 0;
		virtual CRect GetCandidateCommentRect(int id) const = 0;
		virtual CRect GetStatusIconRect() const = 0;

		virtual std::wstring GetLabelText(const std::vector<Text> &labels, int id, const wchar_t *format) const = 0;
		virtual bool IsInlinePreedit() const = 0;
		virtual bool ShouldDisplayStatusIcon() const = 0;
		virtual void GetTextExtentDCMultiline(CDCHandle dc, std::wstring wszString, int nCount, LPSIZE lpSize) const = 0;
		virtual void GetTextSizeDW(const std::wstring text, int nCount, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactaory,  LPSIZE lpSize) const = 0;
		
		virtual std::wstring Layout::ConvertCRLF(std::wstring strString, std::wstring strCRLF) const = 0;
	protected:
		const UIStyle &_style;
		const Context &_context;
		const Status &_status;
	};
};
