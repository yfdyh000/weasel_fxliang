#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include "Layout.h"
#include <Usp10.h>
#include <gdiplus.h>

#include "GdiplusBlur.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace weasel;

typedef CWinTraits<WS_POPUP|WS_CLIPSIBLINGS|WS_DISABLED, WS_EX_TOOLWINDOW|WS_EX_TOPMOST> CWeaselPanelTraits;

typedef enum _backType
{
	TEXT = 0,
	FIRST_CAND,
	MID_CAND,
	LAST_CAND,
	ONLY_CAND,
	NOT_CAND	// background
}BackType;

class WeaselPanel : 
	public CWindowImpl<WeaselPanel, CWindow, CWeaselPanelTraits>,
	CDoubleBufferImpl<WeaselPanel>
{
public:
	BEGIN_MSG_MAP(WeaselPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CDoubleBufferImpl<WeaselPanel>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void CloseDialog(int nVal);

	WeaselPanel(weasel::UI &ui);
	~WeaselPanel();

	void MoveTo(RECT const& rc);
	void Refresh();

	void DoPaint(CDCHandle dc);

private:
	void _CreateLayout();
	void _ResizeWindow();
	void _RepositionWindow();
	bool _DrawPreedit(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawCandidates(CDCHandle dc);
	void _HighlightTextEx(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor, int blurOffsetX, int blurOffsetY, int radius, BackType type );
	void _TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch, IDWriteTextFormat* pTextFormat, FontInfo* pFontInfo);
	HRESULT _TextOutWithFallback_D2D(CDCHandle dc, CRect const rc, std::wstring psz, int cch, COLORREF gdiColor, IDWriteTextFormat* pTextFormat);

	bool _IsHighlightOverCandidateWindow(CRect rc, CRect bg, Gdiplus::Graphics* g);

	weasel::Layout *m_layout;
	weasel::Context &m_ctx;
	weasel::Status &m_status;
	weasel::UIStyle &m_style;

	CRect m_inputPos;
	CIcon m_iconDisabled;
	CIcon m_iconEnabled;
	CIcon m_iconAlpha;
	// for gdiplus drawings
	Gdiplus::GdiplusStartupInput _m_gdiplusStartupInput;
	ULONG_PTR _m_gdiplusToken;
	// for hemispherical dome
	CRect bgRc;
	BYTE m_candidateCount;
	// if hemispherical_dome has been trigged
	bool m_hemispherical_dome = false;

	// for multi font_face & font_point
	GdiplusBlur* m_blurer;
	DirectWriteResources* pDWR;
	GDIFonts* pFonts;

	int offsetX;
	int offsetY;
};

class GraphicsRoundRectPath : public Gdiplus::GraphicsPath
{
public:
	GraphicsRoundRectPath();
	GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery);
	GraphicsRoundRectPath(const CRect rc, int corner);
	GraphicsRoundRectPath(const CRect rc, int corner, bool roundTopLeft, bool roundTopRight, bool roundBottomRight, bool roundBottomLeft);

public:
	void AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery);
};
