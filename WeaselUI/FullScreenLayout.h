#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class FullScreenLayout: public StandardLayout
	{
	public:
		FullScreenLayout(const UIStyle &style, const Context &context, const Status &status, const CRect& inputPos, Layout* layout);
		virtual ~FullScreenLayout();

		virtual void DoLayout(CDCHandle dc, GDIFonts* pFonts = 0);
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR);

	private:
		bool AdjustFontPoint(CDCHandle dc, const CRect& workArea, DirectWriteResources* pDWR, int& step);
		bool AdjustFontPoint(CDCHandle dc, const CRect& workArea, GDIFonts* pFonts, int& step);

		const CRect& mr_inputPos;
		Layout* m_layout;
	};
};
