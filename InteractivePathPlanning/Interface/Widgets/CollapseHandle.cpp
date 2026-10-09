#pragma region Includes

#include "Interface/Widgets/CollapseHandle.h"

#include "Interface/Style/Icons.h"
#include "Interface/Style/Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <memory>

#pragma endregion

namespace
{
	// Widths in DIPs, wider when the panel is closed
	constexpr int OPEN_WIDTH = 16;
	constexpr int COLLAPSED_WIDTH = OPEN_WIDTH + 30;

	// Grip and chevron sizes in DIPs
	constexpr int GRIP_HEIGHT = 44;
	constexpr int GRIP_MAX_WIDTH = 26;
	constexpr int GRIP_INSET = 3;
	constexpr int CHEVRON_SIZE_OPEN = 12;
	constexpr int CHEVRON_SIZE_COLLAPSED = 16;
}

CollapseHandle::CollapseHandle(wxWindow* _parent) : wxWindow(_parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(FromDIP(OPEN_WIDTH), -1));
	SetCursor(wxCursor(wxCURSOR_HAND));

	Bind(wxEVT_PAINT, &CollapseHandle::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &CollapseHandle::OnEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &CollapseHandle::OnLeave, this);
	Bind(wxEVT_LEFT_DOWN, &CollapseHandle::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &CollapseHandle::OnLeftDown, this);
}

void CollapseHandle::SetCollapsed(bool _collapsed)
{
	collapsed = _collapsed;
	SetMinSize(wxSize(FromDIP(collapsed ? COLLAPSED_WIDTH : OPEN_WIDTH), -1));
	Refresh();
}

void CollapseHandle::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	const wxSize size = GetClientSize();

	// highlight the whole strip on hover
	dc.SetBackground(wxBrush(hovered ? Theme::SURFACE_ALT : Theme::PANEL_BACKGROUND));
	dc.Clear();

	// rounded grip in the middle
	const double gripWidth = std::min<double>(size.x - FromDIP(GRIP_INSET) * 2.0, FromDIP(GRIP_MAX_WIDTH));
	const double gripHeight = FromDIP(GRIP_HEIGHT);
	const double gripLeft = (size.x - gripWidth) / 2;
	const double gripTop = (size.y - gripHeight) / 2;

	{
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));

		if (gc)
		{
			// divider line on the left edge
			gc->SetPen(wxPen(Theme::BORDER, 1));
			gc->StrokeLine(0.5, 0, 0.5, size.y);

			gc->SetBrush(wxBrush(Theme::SURFACE));
			gc->DrawRoundedRectangle(gripLeft, gripTop, gripWidth, gripHeight, std::min(gripWidth, gripHeight) / 2);
		}
	}

	// points right when open, left when closed
	const int chevronSize = FromDIP(collapsed ? CHEVRON_SIZE_COLLAPSED : CHEVRON_SIZE_OPEN);
	const char* chevronIcon = collapsed ? Icons::CHEVRON_LEFT : Icons::CHEVRON_RIGHT;
	const wxBitmap chevron = Icons::Render(chevronIcon, hovered ? Theme::ACCENT : Theme::TEXT_MUTED, chevronSize);
	dc.DrawBitmap(chevron, (size.x - chevronSize) / 2, (size.y - chevronSize) / 2, true);
}

void CollapseHandle::OnEnter(wxMouseEvent&)
{
	hovered = true;
	Refresh();
}

void CollapseHandle::OnLeave(wxMouseEvent&)
{
	hovered = false;
	Refresh();
}

void CollapseHandle::OnLeftDown(wxMouseEvent&)
{
	wxCommandEvent event(wxEVT_BUTTON, GetId());
	event.SetEventObject(this);
	ProcessWindowEvent(event);
}
