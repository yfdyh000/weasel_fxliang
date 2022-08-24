#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>

using namespace Gdiplus;
using namespace weasel;
using namespace std;
using namespace boost::algorithm;

static CRect OffsetRect(const CRect rc, int offsetx, int offsety)
{
	CRect res(rc.left + offsetx, rc.top + offsety, rc.right + offsetx, rc.bottom + offsety);
	return res;
}

 WeaselPanel::WeaselPanel(weasel::UI &ui)
	: m_layout(NULL), 
	  m_ctx(ui.ctx()), 
	  m_status(ui.status()), 
	  m_style(ui.style()),
	  m_candidateCount(0),
	  pDWR(NULL),
	  pFonts(NULL),
	  m_blurer(NULL),
	 offsetX(0),
	 offsetY(0),
	  _m_gdiplusToken(0)
{
	m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
}

WeaselPanel::~WeaselPanel()
{
	if (m_layout != NULL)
		delete m_layout;
	if (pDWR != NULL)
		delete pDWR;
	if (pFonts != NULL)
		delete pFonts;
	if (m_blurer != NULL)
		delete m_blurer;
}

void WeaselPanel::_ResizeWindow()
{
	CDCHandle dc = GetDC();
	CSize size = m_layout->GetContentSize();
	int offsetX = 0;
	int offsetY = 0;
	if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
	{
		offsetX = abs(m_style.shadow_offset_x) + m_style.shadow_radius*2;
		offsetY = abs(m_style.shadow_offset_y) + m_style.shadow_radius*2;
		if((!m_style.shadow_offset_x) && (!m_style.shadow_offset_y))
		{
			offsetX *= 2;
			offsetY *= 2;
		}
	}
	size.cx += offsetX*2 + m_style.border*2 + 6;
	size.cy += offsetY*2 + m_style.border*2 + 6;
	SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	ReleaseDC(dc);
}

void WeaselPanel::_CreateLayout()
{
	if (m_layout != NULL)
		delete m_layout;

	Layout* layout = NULL;
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL ||
		m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		layout = new VerticalLayout(m_style, m_ctx, m_status);
	}
	else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new HorizontalLayout(m_style, m_ctx, m_status);
	}
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new FullScreenLayout(m_style, m_ctx, m_status, m_inputPos, layout);
	}
	m_layout = layout;
}

//更新界面
void WeaselPanel::Refresh()
{
	_CreateLayout();

	CDCHandle dc = GetDC();
	if (m_style.color_font)
		m_layout->DoLayout(dc, pDWR);
	else
		m_layout->DoLayout(dc, pFonts);
	ReleaseDC(dc);

	_ResizeWindow();
	// avoid reposition twice
	//_RepositionWindow();
	RedrawWindow();
}
bool WeaselPanel::_IsHighlightOverCandidateWindow(CRect rc, CRect bg, Gdiplus::Graphics* g)
{
	GraphicsRoundRectPath bgPath(bg, m_style.round_corner_ex);
	GraphicsRoundRectPath hlPath(rc, m_style.round_corner);

	Region bgRegion(&bgPath);
	Region hlRegion(&hlPath);
	Region* tmpRegion = hlRegion.Clone();

	tmpRegion->Xor(&bgRegion);
	tmpRegion->Exclude(&bgRegion);
	
	return !tmpRegion->IsEmpty(g);

}
void WeaselPanel::_HighlightTextEx(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor, int blurOffsetX, int blurOffsetY, int radius, BackType type = TEXT)
{
	Graphics gBack(dc);
	gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
	bool rtl = false;
	bool rtr = false;
	bool rbr = false;
	bool rbl = false;
	// if current rc trigger hemispherical dome
	bool hemispherical_dome = false;
	if (type != NOT_CAND && _IsHighlightOverCandidateWindow(rc, bgRc, &gBack))
	{
		m_hemispherical_dome = true;
		CRect trc = bgRc;
		trc.InflateRect(1, 1);
		if(_IsHighlightOverCandidateWindow(rc, trc, &gBack))
			hemispherical_dome = true;
	}
	else
		hemispherical_dome = false;

	// 必须shadow_color都是非完全透明色才做绘制, 全屏状态不绘制阴影保证响应速度
	// 背景状态不需要检查m_hemispherical_dome，避免 MoveTo中的Invalide()影响主窗体阴影绘制
	if ((!m_hemispherical_dome || type == NOT_CAND) && m_style.shadow_radius && (shadowColor & 0xff000000)
		&& m_style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN
		&& m_style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		CRect rect(
			blurOffsetX + m_style.shadow_offset_x,
			blurOffsetY + m_style.shadow_offset_y,
			rc.Width() + blurOffsetX + m_style.shadow_offset_x,
			rc.Height() + blurOffsetY + m_style.shadow_offset_y);
		BYTE r = GetRValue(shadowColor);
		BYTE g = GetGValue(shadowColor);
		BYTE b = GetBValue(shadowColor);
		Color brc = Color::MakeARGB((BYTE)(shadowColor >> 24), r, g, b);
		static Bitmap* pBitmapDropShadow;
		pBitmapDropShadow = new Gdiplus::Bitmap((INT)rc.Width() + blurOffsetX * 2, (INT)rc.Height() + blurOffsetY * 2, PixelFormat32bppARGB);
		Gdiplus::Graphics gg(pBitmapDropShadow);
		gg.SetSmoothingMode(SmoothingModeHighQuality);

		if (m_style.shadow_offset_x != 0 || m_style.shadow_offset_y != 0)
		{
			GraphicsRoundRectPath path(rect, radius);
			SolidBrush br(brc);
			gg.FillPath(&br, &path);
		}
		else
		{
			int pensize = 1;
			int alpha = ((shadowColor >> 24) & 255);
			int step = alpha / m_style.shadow_radius;
			Color scolor = Color::MakeARGB(alpha, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
			Pen penShadow(scolor, (Gdiplus::REAL)pensize);
			CRect rcShadowEx = rect;
			for (int i = 0; i < m_style.shadow_radius; i++)
			{
				GraphicsRoundRectPath path(rcShadowEx, radius + 1 + i);
				gg.DrawPath(&penShadow, &path);
				scolor = Color::MakeARGB(alpha - i * step, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
				penShadow.SetColor(scolor);
				rcShadowEx.InflateRect(2, 2);
			}
		}
		m_blurer->DoGaussianBlur(pBitmapDropShadow, (float)m_style.shadow_radius, (float)m_style.shadow_radius);
		gBack.DrawImage(pBitmapDropShadow, rc.left - blurOffsetX, rc.top - blurOffsetY);
		delete pBitmapDropShadow;
	}
	if (color & 0xff000000)	// 必须back_color非完全透明才绘制
	{
		Color back_color = Color::MakeARGB((color >> 24), GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);
		GraphicsRoundRectPath* bgPath;
		if (m_hemispherical_dome && type!= NOT_CAND && m_style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN && m_style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
		{
			if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL)
			{
				if (m_style.inline_preedit)
				{
					if (type == FIRST_CAND)
					{
						rtl = true; rbl = true; rtr = false; rbr = false;
					}
					else if (type == LAST_CAND)
					{
						rtr = true; rbr = true; rtl = false; rbl = false;
					}
					else if (type == MID_CAND)
					{
						rtl = rtr = rbr = rbl = false;
					}
					else if (type == ONLY_CAND)
					{
						rtl = rtr = rbr = rbl = true;
					}
				}
				else
				{
					if (type == FIRST_CAND)
					{
						rtl = false; rbl = true; rtr = false; rbr = false;
					}
					else if (type == LAST_CAND)
					{
						rtr = false; rbr = true; rtl = false; rbl = false;
					}
					else if (type == MID_CAND)
					{
						rtl = rtr = rbr = rbl = false;
					}
					else if (type == TEXT)
					{
						rtl = hemispherical_dome;
						rtr = rbr = rbl = false;
						if (m_candidateCount == 0)
							rbl = true;
					}
					else if (type == ONLY_CAND)
					{
						rtl = rtr = false;
						rbr = rbl = true;
					}
				}
			}
			else
			{
				if (m_style.inline_preedit)
				{
					if (type == FIRST_CAND)
					{
						rtl = true; rbl = false; rtr = true; rbr = false;
					}
					else if (type == LAST_CAND)
					{
						rtr = false; rbr = true; rtl = false; rbl = true;
					}
					else if (type == MID_CAND)
					{
						rtl = rtr = rbr = rbl = false;
					}
					else if (type == ONLY_CAND)
					{
						rtl = rtr = rbr = rbl = true;
					}
				}
				else
				{
					if (type == FIRST_CAND)
					{
						rtl = rtr = rbr = rbl = false;
					}
					else if (type == LAST_CAND)
					{
						rtr = false; rbr = true; rtl = false; rbl = true;
					}
					else if (type == MID_CAND)
					{
						rtl = rtr = rbr = rbl = false;
					}
					else if (type == TEXT)
					{
						rtl = hemispherical_dome;
						rtr = rbr = rbl = false;
						if (m_candidateCount == 0)
							rbl = true;
					}
					else if (type == ONLY_CAND)
					{
						rtl = rtr = false;
						rbl = rbr = true;
					}
				}
			}
			bgPath = new GraphicsRoundRectPath(rc, m_style.round_corner_ex - (min(m_style.margin_x, m_style.margin_y) - m_style.hilite_padding) - m_style.border / 2 + 1, rtl, rtr, rbr, rbl);
		}
		else
			bgPath = new GraphicsRoundRectPath(rc, radius);
		gBack.FillPath(&gBrBack, bgPath);
	}
}

bool WeaselPanel::_DrawPreedit(Text const& text, CDCHandle dc, CRect const& rc)
{
	bool drawn = false;
	std::wstring const& t = text.str;
	if (!t.empty())
	{
		weasel::TextRange range;
		std::vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
			if (attrs[j].type == weasel::HIGHLIGHTED)
				range = attrs[j].range;

		if (range.start < range.end)
		{
			CSize selStart, selEnd;
			if (m_style.color_font)
			{
				m_layout->GetTextSizeDW(t, range.start, pDWR->pTextFormat, pDWR->pDWFactory, &selStart);
				m_layout->GetTextSizeDW(t, range.end, pDWR->pTextFormat, pDWR->pDWFactory, &selEnd);
			}
			else
			{
				long height = -MulDiv(pFonts->m_TextFont.m_FontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
				CFont font;
				CFontHandle oldFont;
				font.CreateFontW(height, 0, 0, 0, pFonts->m_TextFont.m_FontWeight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFonts->m_TextFont.m_FontFace.c_str());
				oldFont = dc.SelectFont(font);
				m_layout->GetTextExtentDCMultiline(dc, t, range.start, &selStart);
				m_layout->GetTextExtentDCMultiline(dc, t, range.end, &selEnd);
				dc.SelectFont(oldFont);
				font.DeleteObject();
				oldFont.DeleteObject();
			}
			int x = rc.left;
			if (range.start > 0)
			{
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before(x, rc.top, rc.left + selStart.cx, rc.bottom);
				dc.SetTextColor(m_style.text_color);
				if(m_style.color_font)
					_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length(), pDWR->pTextFormat, NULL);
				else
					_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length(), NULL, &pFonts->m_TextFont);
				x += selStart.cx + m_style.hilite_spacing;
			}
			{
				// zzz[yyy]
				std::wstring str_highlight(t.substr(range.start, range.end - range.start));
				CRect rc_hi(x, rc.top, x + (selEnd.cx - selStart.cx), rc.bottom);
				CRect rct = rc_hi;
				rc_hi.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				OffsetRect(rc_hi, -m_style.hilite_padding, 0);
				_HighlightTextEx(dc, rc_hi, m_style.hilited_back_color, m_style.hilited_shadow_color, offsetX * 2, offsetY * 2, m_style.round_corner);
				dc.SetTextColor(m_style.hilited_text_color);
				if(m_style.color_font) 
					_TextOut(dc, x, rc.top, rct, str_highlight.c_str(), str_highlight.length(), pDWR->pTextFormat, NULL);
				else
					_TextOut(dc, x, rc.top, rct, str_highlight.c_str(), str_highlight.length(), NULL, &pFonts->m_TextFont);
				dc.SetTextColor(m_style.text_color);
				dc.SetBkColor(m_style.back_color);
				x += (selEnd.cx - selStart.cx);
			}
			if (range.end < static_cast<int>(t.length()))
			{
				// zzz[yyy]xxx
				x += m_style.hilite_spacing;
				std::wstring str_after(t.substr(range.end));
				dc.SetTextColor(m_style.text_color);
				CRect rc_after(x, rc.top, rc.right, rc.bottom);
				if(m_style.color_font) 
					_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length(), pDWR->pTextFormat, NULL);
				else
					_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length(), NULL, &pFonts->m_TextFont);

			}
		}
		else
		{
			CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
			dc.SetTextColor(m_style.text_color);
			if (m_style.color_font)
				_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length(), pDWR->pTextFormat, NULL);
			else
				_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length(), NULL, &pFonts->m_TextFont);
		}
		drawn = true;
	}
	return drawn;
}

bool WeaselPanel::_DrawCandidates(CDCHandle dc)
{
	bool drawn = false;
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	const std::vector<Text> &comments(m_ctx.cinfo.comments);
	const std::vector<Text> &labels(m_ctx.cinfo.labels);

	int bkx = 2 * offsetX;
	int bky = 2 * offsetY;

	BackType bkType = FIRST_CAND;
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		if (candidates.size() == 1)
			bkType = ONLY_CAND;
		else if (i != 0 && i != candidates.size() - 1)
			bkType = MID_CAND;
		else if (i == candidates.size() - 1)
			bkType = LAST_CAND;
		CRect rect;
		rect = OffsetRect(m_layout->GetCandidateRect(i), offsetX, offsetY);
		rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
		if (i == m_ctx.cinfo.highlighted)
		{
			_HighlightTextEx(dc, rect, m_style.hilited_candidate_back_color, m_style.hilited_candidate_shadow_color, bkx, bky, m_style.round_corner, bkType);
			dc.SetTextColor(m_style.hilited_label_text_color);
		}
		else
		{
			_HighlightTextEx(dc, rect, m_style.candidate_back_color, m_style.candidate_shadow_color, bkx, bky, m_style.round_corner, bkType);
			dc.SetTextColor(m_style.label_text_color);
		}

		// Draw label
		std::wstring label = m_layout->GetLabelText(labels, i, m_style.label_text_format.c_str());
		rect = m_layout->GetCandidateLabelRect(i);
		rect = OffsetRect(rect, offsetX, offsetY);

		if (m_style.color_font)
			_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length(), pDWR->pLabelTextFormat, NULL);
		else
			_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length(), NULL, &pFonts->m_LabelFont);

		// Draw text
		std::wstring text = candidates.at(i).str;
		if (i == m_ctx.cinfo.highlighted)
			dc.SetTextColor(m_style.hilited_candidate_text_color);
		else
			dc.SetTextColor(m_style.candidate_text_color);
		rect = m_layout->GetCandidateTextRect(i);
		rect = OffsetRect(rect, offsetX, offsetY);
		if (m_style.color_font)
			_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length(), pDWR->pTextFormat, NULL);
		else
			_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length(), NULL, &pFonts->m_TextFont);
		
		// Draw comment
		std::wstring comment = comments.at(i).str;
		if (!comment.empty())
		{
			if (i == m_ctx.cinfo.highlighted)
				dc.SetTextColor(m_style.hilited_comment_text_color);
			else
				dc.SetTextColor(m_style.comment_text_color);
			rect = m_layout->GetCandidateCommentRect(i);
			rect = OffsetRect(rect, offsetX, offsetY);
			if(m_style.color_font)
				_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length(), pDWR->pCommentTextFormat, NULL);
			else
				_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length(), NULL, &pFonts->m_CommentFont);
		}
		drawn = true;
	}

	dc.SetTextColor(m_style.text_color);
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	// background start
	CRect rc;
	GetClientRect(&rc);

	SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };
	CDCHandle hdc = ::GetDC(m_hWnd);
	CDCHandle memDC = ::CreateCompatibleDC(hdc);
	HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, sz.cx, sz.cy);
	::SelectObject(memDC, memBitmap);
	ReleaseDC(hdc);
	
	// background start
	/* inline_preedit and candidate size 1 and preedit_type preview, and hide_candidates_when_single is set */
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	m_candidateCount = candidates.size();
	bool hide_candidates = false;
	if (m_style.hide_candidates_when_single == True 
		&& m_style.inline_preedit == True 
		&& candidates.size() == 1 )
		hide_candidates = True;

	CRect trc;
	/* (candidate not empty or (input not empty and not inline_preedit)) and not hide_candidates */
	if( (!(candidates.size()==0) 
			|| ((!m_ctx.aux.str.empty() || !m_ctx.preedit.str.empty()) && !m_style.inline_preedit)) 
		&& !hide_candidates)
	{
		Graphics gBack(memDC);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		trc = rc;
		trc.DeflateRect(offsetX-m_style.border/2, offsetY-m_style.border/2);
		bgRc = trc;
		bgRc.DeflateRect(m_style.border / 2 + 1, m_style.border / 2 + 1);
		GraphicsRoundRectPath bgPath(trc, m_style.round_corner_ex);
		int alpha = ((m_style.border_color >> 24) & 255);
		Color border_color = Color::MakeARGB(alpha, GetRValue(m_style.border_color), GetGValue(m_style.border_color), GetBValue(m_style.border_color));
		Pen gPenBorder(border_color, (Gdiplus::REAL)m_style.border);
		trc.InflateRect(m_style.border / 2, m_style.border / 2);
		_HighlightTextEx(memDC, trc, m_style.back_color, m_style.shadow_color, offsetX * 2, offsetY * 2, m_style.round_corner_ex + m_style.border / 2, NOT_CAND);
		if(m_style.border)
			gBack.DrawPath(&gPenBorder, &bgPath);
		gBack.ReleaseHDC(memDC);
	}
	// background end

	bool drawn = false;

	// draw preedit string
	if (!m_layout->IsInlinePreedit() && !hide_candidates)
	{
		trc = OffsetRect(m_layout->GetPreeditRect(), offsetX, offsetY);
		drawn |= _DrawPreedit(m_ctx.preedit, memDC, trc);
	}
	
	// draw auxiliary string
	drawn |= _DrawPreedit(m_ctx.aux, memDC, m_layout->GetAuxiliaryRect());

	// status icon (I guess Metro IME stole my idea :)
	if (m_layout->ShouldDisplayStatusIcon())
	{
		const CRect iconRect(OffsetRect(m_layout->GetStatusIconRect(), offsetX, offsetY));
		CIcon& icon(m_status.disabled ? m_iconDisabled : m_status.ascii_mode ? m_iconAlpha : m_iconEnabled);
		memDC.DrawIconEx(iconRect.left, iconRect.top, icon, 0, 0);
		drawn = true;
	}

	// draw candidates
	if(!hide_candidates)
		drawn |= _DrawCandidates(memDC);

	/* Nothing drawn, hide candidate window */
	if (!drawn)
		ShowWindow(SW_HIDE);

	HDC screenDC = ::GetDC(NULL);
	CRect rect;
	GetWindowRect(&rect);
	POINT ptSrc = {
		rect.left,
		rect.top 
	};
	POINT ptDest = {
		rc.left,
		rc.top 
	};

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;
	::UpdateLayeredWindow(m_hWnd, screenDC, &ptSrc, &sz, memDC, &ptDest, RGB(0,0,0), &bf, ULW_ALPHA);
	::DeleteDC(memDC);
	::DeleteObject(memBitmap);
	ReleaseDC(screenDC);

}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG t = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	t |= WS_EX_LAYERED|WS_EX_TRANSPARENT;
	::SetWindowLong(m_hWnd, GWL_EXSTYLE, t);
	GdiplusStartup(&_m_gdiplusToken, &_m_gdiplusStartupInput, NULL);
	GetWindowRect(&m_inputPos);
	if (m_style.color_font)
	{
		// prepare d2d1 resources
		pDWR = new DirectWriteResources(m_style);
	}
	else
	{
		pFonts = new GDIFonts(&m_style);
	}
	m_blurer = new GdiplusBlur();
	// main offset because of window shadow
	offsetX = offsetY = 0;
	if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
	{
		offsetX = abs(m_style.shadow_offset_x) + m_style.shadow_radius*2;
		offsetY = abs(m_style.shadow_offset_y) + m_style.shadow_radius*2;
		if((!m_style.shadow_offset_x) && (!m_style.shadow_offset_y))
		{
			offsetX *= 2;
			offsetY *= 2;
		}
	}
	offsetX += m_style.border + 3;
	offsetY += m_style.border + 3;

	Refresh();
	return TRUE;
}

LRESULT WeaselPanel::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	GdiplusShutdown(_m_gdiplusToken);
	return 0;
}

void WeaselPanel::CloseDialog(int nVal)
{
	
}

void WeaselPanel::MoveTo(RECT const& rc)
{
	const int distance = 3;
	m_inputPos = rc;
	m_inputPos.OffsetRect(0, distance);
	_RepositionWindow();
	// invalidate for drawing right after keydown got, for layeredwindow
	Invalidate();
}

void WeaselPanel::_RepositionWindow()
{
	RECT rcWorkArea;
	//SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	memset(&rcWorkArea, 0, sizeof(rcWorkArea));
	HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			rcWorkArea = info.rcWork;
		}
	}
	RECT rcWindow;
	GetWindowRect(&rcWindow);
	int width = (rcWindow.right - rcWindow.left);
	int height = (rcWindow.bottom - rcWindow.top);
	// keep panel visible
	rcWorkArea.right -= width;
	rcWorkArea.bottom -= height;
	int x = m_inputPos.left;
	int y = m_inputPos.bottom;
	if (x > rcWorkArea.right)
		x = rcWorkArea.right;
	if (x < rcWorkArea.left)
		x = rcWorkArea.left;
	// show panel above the input focus if we're around the bottom
	if (y > rcWorkArea.bottom)
		y = m_inputPos.top - height;
	if (y > rcWorkArea.bottom)
		y = rcWorkArea.bottom;
	if (y < rcWorkArea.top)
		y = rcWorkArea.top;
	// memorize adjusted position (to avoid window bouncing on height change)
	bool backShadowEnable = (m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0);
	if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN || m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		x -= (offsetX + 1) * (int)backShadowEnable + 3 * (int)(!backShadowEnable);
		y -= (offsetY + 1) * (int)backShadowEnable + 3 * (int)(!backShadowEnable);
	}
	else 
	{
		bool roundShadowEnable = m_style.shadow_offset_x == 0 && m_style.shadow_offset_y == 0;
		int offsetx = 0;
		int offsety = 0;
		if(backShadowEnable)
		{
			if (roundShadowEnable)
			{
				offsetx = offsety = m_style.shadow_radius*2;
			}
			else
			{
				offsetx = (m_style.shadow_offset_x < 0) ? m_style.shadow_radius*2 : (m_style.shadow_offset_x + m_style.shadow_radius*2);
				offsety = (m_style.shadow_offset_y < 0) ? m_style.shadow_radius*2 : (m_style.shadow_offset_y + m_style.shadow_radius*2);
			}
			x -= offsetx;
			y -= offsety;
		}
	}

	m_inputPos.bottom = y;
	SetWindowPos(HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
}

static HRESULT _TextOutWithFallback(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;

    hr = ScriptStringAnalyse(
        dc,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {
        hr = ScriptStringOut(
            ssa, x, y, 0,
            &rc,
            0, 0, FALSE);
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

static HBITMAP _CreateAlphaTextBitmap(LPCWSTR inText, const CFont& inFont, COLORREF inColor, int cch)
{
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
	HBITMAP hMyDIB = NULL;
	// get text area
	RECT TextArea = { 0, 0, 0, 0 };
	DrawText(hTextDC, inText, cch, &TextArea, DT_CALCRECT);
	if ((TextArea.right > TextArea.left) && (TextArea.bottom > TextArea.top))
	{
		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		// dib setup
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = TextArea.right - TextArea.left;
		BMIH.biHeight = TextArea.bottom - TextArea.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;

		// create and select dib into dc
		hMyDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTextDC, hMyDIB);
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			DrawText(hTextDC, inText, cch, &TextArea, DT_NOCLIP);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;

			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
	}
	SelectObject(hTextDC, hOldFont);
	DeleteDC(hTextDC);
	return hMyDIB;
}

static HRESULT _TextOutWithFallback_ULW(CDCHandle dc, int x, int y, CRect const rc, LPCWSTR psz, int cch, const CFont& font, const COLORREF& inColor, BYTE alpha)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;
	int TextLength = cch;
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, font);
	HBITMAP MyBMP = NULL;

    hr = ScriptStringAnalyse(
        hTextDC,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {

		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = rc.right - rc.left;
		BMIH.biHeight = rc.bottom - rc.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;
		MyBMP = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP;
		if(MyBMP)
			hOldBMP = (HBITMAP)SelectObject(hTextDC, MyBMP);
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			hr = ScriptStringOut(ssa, 0, 0, 0, rc, 0, 0, FALSE);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;
			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
		SelectObject(hTextDC, hOldFont);
		DeleteDC(hTextDC);
		if (MyBMP)
		{
			// temporary dc select bmp into it
			HDC hTempDC = CreateCompatibleDC(dc);
			HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
			if (hOldBMP)
			{
				BITMAP BMInf;
				GetObject(MyBMP, sizeof(BITMAP), &BMInf);
				// fill blend function and blend new text to window
				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.SourceConstantAlpha = alpha;
				bf.AlphaFormat = AC_SRC_ALPHA;
				AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
				// clean up
				SelectObject(hTempDC, hOldBMP);
				DeleteObject(MyBMP);
				DeleteDC(hTempDC);
			}
		}
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

static void _AlphaBlendBmpToDC(CDCHandle& dc, const int x, const int y, const BYTE alpha, const HBITMAP& MyBMP)
{
	HDC hTempDC = CreateCompatibleDC(dc);
	HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
	if (hOldBMP)
	{
		BITMAP BMInf;
		GetObject(MyBMP, sizeof(BITMAP), &BMInf);
		// fill blend function and blend new text to window
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = alpha;
		bf.AlphaFormat = AC_SRC_ALPHA;
		AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
		// clean up
		SelectObject(hTempDC, hOldBMP);
		DeleteObject(MyBMP);
		DeleteDC(hTempDC);
	}
}

HRESULT WeaselPanel::_TextOutWithFallback_D2D (CDCHandle dc, CRect const rc, wstring psz, int cch, COLORREF gdiColor, IDWriteTextFormat* pTextFormat)
{
	float r = (float)(GetRValue(gdiColor))/255.0f;
	float g = (float)(GetGValue(gdiColor))/255.0f;
	float b = (float)(GetBValue(gdiColor))/255.0f;

	// alpha 
	float alpha = (float)((gdiColor >> 24) & 255) / 255.0f;
	ID2D1SolidColorBrush* pBrush = NULL;
	pDWR->pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, alpha), &pBrush);
	if (NULL != pBrush && NULL != pDWR->pTextFormat)
	{
		IDWriteTextLayout* pTextLayout = NULL;
		if (pTextFormat == NULL)
			pTextFormat = pDWR->pTextFormat;
		pDWR->pDWFactory->CreateTextLayout( psz.c_str(), psz.size(), pTextFormat, rc.Width(), rc.Height(), &pTextLayout);
		float offsetx = 0;
		float offsety = 0;
		DWRITE_OVERHANG_METRICS omt;
		pTextLayout->GetOverhangMetrics(&omt);
		if (omt.left > 0)
			offsetx += omt.left;
		pDWR->pRenderTarget->BindDC(dc, &rc);
		pDWR->pRenderTarget->BeginDraw();
		if (pTextLayout != NULL)
			pDWR->pRenderTarget->DrawTextLayout({ offsetx, offsety}, pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		// for rect checking
		//D2D1_RECT_F rectf{ 0,0, rc.Width(), rc.Height() };
		//pDWR->pRenderTarget->DrawRectangle(&rectf, pBrush);
		pDWR->pRenderTarget->EndDraw();
		SafeRelease(&pTextLayout);
	}
	pBrush->Release();
	return S_OK;
}

void WeaselPanel::_TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch, IDWriteTextFormat* pTextFormat, FontInfo* pFontInfo)
{
	if (m_style.color_font )
	{
		_TextOutWithFallback_D2D(dc, rc, psz, cch, dc.GetTextColor(), pTextFormat);
	}
	else
	{ 
		CFont font;
		CSize size;
		long height = -MulDiv(pFontInfo->m_FontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
		font.CreateFontW(height, 0, 0, 0, pFontInfo->m_FontWeight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFontInfo->m_FontFace.c_str());
		dc.SelectFont(font);
		COLORREF inColor = dc.GetTextColor();
		BYTE alpha = (inColor >> 24) & 255 ;
		int offset = 0;
		// split strings with \r, for multiline string drawing
		std::vector<std::wstring> lines;
		split(lines, psz, is_any_of(L"\r"));
		for (wstring line : lines)
		{
			// calc line size, for y offset calc
			dc.GetTextExtent(line.c_str(), line.length(), &size);
			if (FAILED(_TextOutWithFallback_ULW(dc, x, y+offset, rc, line.c_str(), line.length(), font, inColor, alpha))) 
			{
				HBITMAP MyBMP = _CreateAlphaTextBitmap(psz, font, dc.GetTextColor(), cch);
				if (MyBMP)
				{
					_AlphaBlendBmpToDC(dc, x, y + offset, alpha, MyBMP);
				}
			}
			offset += size.cy;
		}
	}
}
GraphicsRoundRectPath::GraphicsRoundRectPath(void) : Gdiplus::GraphicsPath()
{

}
GraphicsRoundRectPath::GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery) 
	: Gdiplus::GraphicsPath()
{
	AddRoundRect(left, top, width, height, cornerx, cornery);
}
GraphicsRoundRectPath::GraphicsRoundRectPath(const CRect rc, int corner)
{
	if (corner > 0)
		AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), corner, corner);
	else
		AddRectangle(Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()));
}
GraphicsRoundRectPath::GraphicsRoundRectPath(const CRect rc, int corner, bool roundTopLeft, bool roundTopRight, bool roundBottomRight, bool roundBottomLeft)
{
	if (!(roundTopLeft || roundTopRight || roundBottomRight || roundBottomLeft) || corner <= 0)
	{
		Rect& rcp = Rect(rc.left, rc.top, rc.Width()  , rc.Height());
		AddRectangle(rcp);
	}
	else
	{
		int cnx = ((corner * 2 <= rc.Width()) ? corner : (rc.Width() / 2));
		int cny = ((corner * 2 <= rc.Height()) ? corner : (rc.Height() / 2));
		int elWid = 2 * cnx;
		int elHei = 2 * cny;
		AddArc(rc.left, rc.top, elWid * roundTopLeft, elHei * roundTopLeft, 180, 90);
		AddLine(rc.left + cnx * roundTopLeft, rc.top, rc.right - cnx * roundTopRight, rc.top);

		AddArc(rc.right - elWid * roundTopRight, rc.top, elWid * roundTopRight, elHei * roundTopRight, 270, 90);
		AddLine(rc.right, rc.top + cny * roundTopRight, rc.right, rc.bottom - cny * roundBottomRight);

		AddArc(rc.right - elWid * roundBottomRight, rc.bottom - elHei * roundBottomRight, elWid * roundBottomRight, elHei * roundBottomRight, 0, 90);
		AddLine(rc.right - cnx * roundBottomRight, rc.bottom, rc.left + cnx * roundBottomLeft, rc.bottom);

		AddArc(rc.left, rc.bottom - elHei * roundBottomLeft, elWid * roundBottomLeft, elHei * roundBottomLeft, 90, 90);
		AddLine(rc.left, rc.top + cny * roundTopLeft - 1, rc.left, rc.bottom - cny * roundBottomLeft);
	}
}

void GraphicsRoundRectPath::AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery)
{
	if (cornery > 0 && cornerx > 0)
	{
		int cnx = ((cornerx * 2 <= width) ? cornerx : (width / 2));
		int cny = ((cornery * 2 <= height) ? cornery : (height / 2));
		int elWid = 2 * cnx;
		int elHei = 2 * cny;
		AddArc(left, top, elWid, elHei, 180, 90);
		AddLine(left + cnx, top, left + width - cnx, top);

		AddArc(left + width - elWid, top, elWid, elHei, 270, 90);
		AddLine(left + width, top + cny, left + width, top + height - cny);

		AddArc(left + width - elWid, top + height - elHei, elWid, elHei, 0, 90);
		AddLine(left + width - cnx, top + height, left + cnx, top + height);

		AddArc(left, top + height - elHei, elWid, elHei, 90, 90);
		AddLine(left, top + cny-1, left, top + height - cny);
	}
	else
	{
		Gdiplus::Rect& rc = Gdiplus::Rect(left, top, width, height);
		AddRectangle(rc);
	}
}
