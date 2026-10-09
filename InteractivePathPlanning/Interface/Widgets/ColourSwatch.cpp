#pragma region Includes

#include "Interface/Widgets/ColourSwatch.h"

#include "Interface/Style/Theme.h"

#include <wx/clrpicker.h>
#include <wx/colordlg.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <memory>

#pragma endregion

namespace
{
	// Sizes in DIPs
	constexpr int SWATCH_WIDTH = 48;
	constexpr int SWATCH_HEIGHT = 26;
	constexpr int CORNER_RADIUS = 6;
}

ColourSwatch::ColourSwatch(wxWindow* _parent) : wxWindow(_parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
, colour(*wxBLACK)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(FromDIP(wxSize(SWATCH_WIDTH, SWATCH_HEIGHT)));
	SetCursor(wxCursor(wxCURSOR_HAND));

	Bind(wxEVT_PAINT, &ColourSwatch::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &ColourSwatch::OnEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &ColourSwatch::OnLeave, this);
	Bind(wxEVT_LEFT_DOWN, &ColourSwatch::OnLeftDown, this);
}

void ColourSwatch::SetColour(const wxColour& _colour)
{
	colour = _colour;
	Refresh();
}

void ColourSwatch::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
	dc.Clear();

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));

	if (!gc)
	{
		return;
	}

	// thicker accent outline on hover
	const double outlineWidth = hovered ? FromDIP(2) : 1.0;
	const wxSize size = GetClientSize();

	gc->SetPen(wxPen(hovered ? Theme::ACCENT : Theme::BORDER, static_cast<int>(outlineWidth)));
	gc->SetBrush(wxBrush(colour));
	gc->DrawRoundedRectangle(outlineWidth / 2, outlineWidth / 2, size.x - outlineWidth, size.y - outlineWidth, FromDIP(CORNER_RADIUS));
}

void ColourSwatch::OnEnter(wxMouseEvent&)
{
	hovered = true;
	Refresh();
}

void ColourSwatch::OnLeave(wxMouseEvent&)
{
	hovered = false;
	Refresh();
}

void ColourSwatch::OnLeftDown(wxMouseEvent&)
{
	wxColourData data;
	data.SetColour(colour);
	data.SetChooseFull(true);

	wxColourDialog chooser(this, &data);

	if (chooser.ShowModal() != wxID_OK || chooser.GetColourData().GetColour() == colour)
	{
		return;
	}

	SetColour(chooser.GetColourData().GetColour());

	wxColourPickerEvent event(this, GetId(), colour);
	ProcessWindowEvent(event);
}
