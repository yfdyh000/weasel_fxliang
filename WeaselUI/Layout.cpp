#include "stdafx.h"
#include "Layout.h"

Layout::Layout(const UIStyle& style, const Context& context, const Status& status)
	: _style(style), _context(context), _status(status)
{
	offsetX = offsetY = 0;
	if(_style.shadow_color & 0xff000000 && _style.shadow_radius != 0)
	{
		offsetX = abs(_style.shadow_offset_x) + _style.shadow_radius*2;
		offsetY = abs(_style.shadow_offset_y) + _style.shadow_radius*2;
		if((!_style.shadow_offset_x) && (!_style.shadow_offset_y))
		{
			offsetX *= 2;
			offsetY *= 2;
		}
	}
	offsetX += _style.border;
	offsetY += _style.border;
}
