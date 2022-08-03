#include "stdafx.h"
#include "FullScreenLayout.h"

using namespace weasel;

FullScreenLayout::FullScreenLayout(const UIStyle &style, const Context &context, const Status &status, const CRect& inputPos, Layout* layout)
	: StandardLayout(style, context, status), mr_inputPos(inputPos), m_layout(layout)
{
}

FullScreenLayout::~FullScreenLayout()
{
	delete m_layout;
}

void FullScreenLayout::DoLayout(CDCHandle dc, GDIFonts* pFonts)
{
	if (_context.empty())
	{
		int width = 0, height = 0;
		UpdateStatusIconLayout(&width, &height);
		_contentSize.SetSize(width, height);
		return;
	}

	CRect workArea;
	HMONITOR hMonitor = MonitorFromRect(mr_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			workArea = info.rcWork;
		}
	}

	int step = 32;
	do {
		m_layout->DoLayout(dc, pFonts);
	}
	while (AdjustFontPoint(dc, workArea, pFonts, step));

	int offsetX = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
	int offsetY = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
	_preeditRect = m_layout->GetPreeditRect();
	_preeditRect.OffsetRect(offsetX, offsetY);
	_auxiliaryRect = m_layout->GetAuxiliaryRect();
	_auxiliaryRect.OffsetRect(offsetX, offsetY);
	_highlightRect = m_layout->GetHighlightRect();
	_highlightRect.OffsetRect(offsetX, offsetY);
	for (int i = 0, n = (int)_context.cinfo.candies.size(); i < n && i < MAX_CANDIDATES_COUNT; ++i)
	{
		_candidateLabelRects[i] = m_layout->GetCandidateLabelRect(i);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		_candidateTextRects[i] = m_layout->GetCandidateTextRect(i);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		_candidateCommentRects[i] = m_layout->GetCandidateCommentRect(i);
		_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
	}
	_statusIconRect = m_layout->GetStatusIconRect();
	_statusIconRect.OffsetRect(offsetX, offsetY);

	_contentSize.SetSize(workArea.Width(), workArea.Height());
}

void weasel::FullScreenLayout::DoLayout(CDCHandle dc, DirectWriteResources* pDWR)
{
	if (_context.empty())
	{
		int width = 0, height = 0;
		UpdateStatusIconLayout(&width, &height);
		_contentSize.SetSize(width, height);
		return;
	}

	CRect workArea;
	HMONITOR hMonitor = MonitorFromRect(mr_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			workArea = info.rcWork;
		}
	}

	int step = 32;
	do {
		m_layout->DoLayout(dc, pDWR);
	}
	while (AdjustFontPoint(dc, workArea, pDWR, step));

	int offsetX = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
	int offsetY = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
	_preeditRect = m_layout->GetPreeditRect();
	_preeditRect.OffsetRect(offsetX, offsetY);
	_auxiliaryRect = m_layout->GetAuxiliaryRect();
	_auxiliaryRect.OffsetRect(offsetX, offsetY);
	_highlightRect = m_layout->GetHighlightRect();
	_highlightRect.OffsetRect(offsetX, offsetY);
	for (int i = 0, n = (int)_context.cinfo.candies.size(); i < n && i < MAX_CANDIDATES_COUNT; ++i)
	{
		_candidateLabelRects[i] = m_layout->GetCandidateLabelRect(i);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		_candidateTextRects[i] = m_layout->GetCandidateTextRect(i);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		_candidateCommentRects[i] = m_layout->GetCandidateCommentRect(i);
		_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
	}
	_statusIconRect = m_layout->GetStatusIconRect();
	_statusIconRect.OffsetRect(offsetX, offsetY);

	_contentSize.SetSize(workArea.Width(), workArea.Height());
}

bool FullScreenLayout::AdjustFontPoint(CDCHandle dc, const CRect& workArea, DirectWriteResources* pDWR, int& step)
{
	if (_context.empty() || step == 0)
		return false;
	int fontPointLabel		= pDWR->pLabelTextFormat->GetFontSize() / pDWR->dpiScaleX_;
	int fontPoint			= pDWR->pTextFormat->GetFontSize() / pDWR->dpiScaleX_;
	int fontPointComment	= pDWR->pCommentTextFormat->GetFontSize() / pDWR->dpiScaleX_;
	CSize sz = m_layout->GetContentSize();
	if (sz.cx > workArea.Width() || sz.cy > workArea.Height())
	{
		if (step > 0)
		{
			step = - (step >> 1);
		}
		fontPoint += step;
		fontPointLabel += step;
		fontPointComment += step;
		if(fontPoint < 4) fontPoint = 4; 
		if(fontPointLabel < 4) fontPointLabel = 4; 
		if(fontPointComment < 4) fontPointComment = 4; 
		pDWR->InitResources(_style.label_font_face, fontPointLabel, _style.font_face, fontPoint, _style.comment_font_face, fontPointComment, _style.align_type);
		return true;
	}
	else if (sz.cx <= workArea.Width() * 31 / 32 && sz.cy <= workArea.Height() * 31 / 32)
	{
		if (step < 0)
		{
			step = -step >> 1;
		}
		fontPoint += step;
		fontPointLabel += step;
		fontPointComment += step;
		if(fontPoint < 4) fontPoint = 4; 
		if(fontPointLabel < 4) fontPointLabel = 4; 
		if(fontPointComment < 4) fontPointComment = 4; 
		pDWR->InitResources(_style.label_font_face, fontPointLabel, _style.font_face, fontPoint, _style.comment_font_face, fontPointComment, _style.align_type);
		return true;
	}

	return false;
}

bool weasel::FullScreenLayout::AdjustFontPoint(CDCHandle dc, const CRect& workArea, GDIFonts* pFonts, int& step)
{
	if (_context.empty() || step == 0)
		return false;

	CSize sz = m_layout->GetContentSize();
	if (sz.cx > workArea.Width() || sz.cy > workArea.Height())
	{
		if (step > 0)
		{
			step = - (step >> 1);
		}
		pFonts->m_LabelFont.m_FontPoint += step;
		pFonts->m_TextFont.m_FontPoint += step;
		pFonts->m_CommentFont.m_FontPoint += step;
		return true;
	}
	else if (sz.cx <= workArea.Width() * 31 / 32 && sz.cy <= workArea.Height() * 31 / 32)
	{
		if (step < 0)
		{
			step = -step >> 1;
		}
		pFonts->m_LabelFont.m_FontPoint += step;
		pFonts->m_TextFont.m_FontPoint += step;
		pFonts->m_CommentFont.m_FontPoint += step;
		return true;
	}

	return false;
}
