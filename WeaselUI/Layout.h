#pragma once

#include <WeaselCommon.h>
#include <WeaselUI.h>
#include <d2d1.h>
#include <dwrite_2.h>
#include <vector>
#include <boost/algorithm/string.hpp>

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
		void _SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector);
		HRESULT _SetTextFormat(IDWriteTextFormat1* pTextFormat, const wstring fontFace, const UINT32 fontPoint, const UIStyle::LayoutAlignType alignType = UIStyle::ALIGN_BOTTOM);
	};

	class GDIFonts
	{
	public:
		//GDIFonts() {};
		~GDIFonts() {}
		GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint);
		wstring _LabelFontFace;
		wstring _TextFontFace;
		wstring _CommentFontFace;
		int _LabelFontPoint;
		int _TextFontPoint;
		int _CommentFontPoint;
	};

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

		virtual wstring GetLabelText(const vector<Text> &labels, int id, const wchar_t *format) const = 0;
		virtual bool IsInlinePreedit() const = 0;
		virtual bool ShouldDisplayStatusIcon() const = 0;
		virtual void GetTextExtentDCMultiline(CDCHandle dc, wstring wszString, int nCount, LPSIZE lpSize) const = 0;
		virtual void GetTextSizeDW(const wstring text, int nCount, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactaory,  LPSIZE lpSize) const = 0;
		
		virtual wstring Layout::ConvertCRLF(wstring strString, wstring strCRLF) const = 0;
	protected:
		const UIStyle &_style;
		const Context &_context;
		const Status &_status;
	};
};
