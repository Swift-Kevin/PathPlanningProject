#pragma region Includes

#include "Interface/Widgets/FlatButton.h"

#include "Interface/Style/Icons.h"
#include "Interface/Style/Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <memory>

#pragma endregion

namespace
{
	// Sizes in DIPs
	constexpr int BUTTON_HEIGHT = 30;
	constexpr int PADDING_X = 12;
	constexpr int CHEVRON_SIZE = 12;
	constexpr int CHEVRON_GAP = 8;
	constexpr int CORNER_RADIUS = 7;
}

FlatButton::FlatButton(wxWindow* _parent, const wxString& _text, bool _primary) : wxWindow(_parent, wxID_ANY)
, text(_text), primary(_primary)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetCursor(wxCursor(wxCURSOR_HAND));

	Bind(wxEVT_PAINT, &FlatButton::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &FlatButton::OnEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &FlatButton::OnLeave, this);
	Bind(wxEVT_LEFT_DOWN, &FlatButton::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &FlatButton::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &FlatButton::OnLeftUp, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &FlatButton::OnCaptureLost, this);
}

void FlatButton::SetText(const wxString& _text)
{
	if (text == _text)
	{
		return;
	}

	text = _text;
	InvalidateBestSize();
	Refresh();
}

void FlatButton::DoEnable(bool _enable)
{
	wxWindow::DoEnable(_enable);

	// redraw with the disabled or normal colours
	hovered = false;
	Refresh();
}

void FlatButton::SetDropdown(bool _dropdown)
{
	dropdown = _dropdown;
	InvalidateBestSize();
	Refresh();
}

wxSize FlatButton::DoGetBestClientSize() const
{
	int textWidth = 0;
	int textHeight = 0;
	GetTextExtent(text, &textWidth, &textHeight);

	int width = textWidth + FromDIP(PADDING_X) * 2;

	if (dropdown)
	{
		width += FromDIP(CHEVRON_GAP) + FromDIP(CHEVRON_SIZE);
	}

	return wxSize(width, FromDIP(BUTTON_HEIGHT));
}

#pragma region Events

void FlatButton::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	const wxSize size = GetClientSize();

	// fill the corners with the parent's colour
	dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
	dc.Clear();

	// colours for the current state
	wxColour fill = Theme::SURFACE_ALT;
	wxColour outline = Theme::BORDER;
	wxColour foreground = Theme::TEXT;

	if (!IsEnabled())
	{
		foreground = Theme::TEXT_DISABLED;
	}
	else if (primary)
	{
		fill = hovered ? Theme::ACCENT_HOVER : Theme::ACCENT;
		outline = fill;
		foreground = *wxWHITE;
	}
	else if (hovered)
	{
		fill = Theme::ACCENT_SOFT;
		outline = Theme::ACCENT;
		foreground = Theme::ACCENT;
	}

	if (pressed && hovered)
	{
		fill = fill.ChangeLightness(92);
	}

	{
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));

		if (gc)
		{
			gc->SetPen(wxPen(outline, 1));
			gc->SetBrush(wxBrush(fill));
			gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, FromDIP(CORNER_RADIUS));
		}
	}

	// draw text with the DC to keep ClearType
	dc.SetFont(GetFont());
	dc.SetTextForeground(foreground);
	const wxSize textSize = dc.GetTextExtent(text);
	const int textY = (size.y - textSize.y) / 2;

	if (!dropdown)
	{
		dc.DrawText(text, (size.x - textSize.x) / 2, textY);
		return;
	}

	const int chevronSize = FromDIP(CHEVRON_SIZE);
	wxColour chevronColour = hovered ? Theme::ACCENT : Theme::TEXT_MUTED;

	if (!IsEnabled())
	{
		chevronColour = Theme::TEXT_DISABLED;
	}

	const wxBitmap chevron = Icons::Render(Icons::CHEVRON_DOWN, chevronColour, chevronSize);
	dc.DrawText(text, FromDIP(PADDING_X), textY);
	dc.DrawBitmap(chevron, size.x - FromDIP(PADDING_X) - chevronSize, (size.y - chevronSize) / 2, true);
}

void FlatButton::OnEnter(wxMouseEvent&)
{
	hovered = true;
	Refresh();
}

void FlatButton::OnLeave(wxMouseEvent&)
{
	hovered = false;
	Refresh();
}

void FlatButton::OnLeftDown(wxMouseEvent&)
{
	pressed = true;

	if (!HasCapture())
	{
		CaptureMouse();
	}

	Refresh();
}

void FlatButton::OnLeftUp(wxMouseEvent& _event)
{
	if (HasCapture())
	{
		ReleaseMouse();
	}

	// only counts as a click if released over the button
	const bool clicked = pressed && GetClientRect().Contains(_event.GetPosition());
	pressed = false;
	Refresh();

	if (clicked)
	{
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}
}

void FlatButton::OnCaptureLost(wxMouseCaptureLostEvent&)
{
	pressed = false;
	Refresh();
}

#pragma endregion
