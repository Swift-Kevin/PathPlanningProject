#pragma region Includes

#include "Interface/Widgets/Card.h"

#include "Interface/Style/Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <memory>

#pragma endregion

namespace
{
	// Sizes in DIPs
	constexpr int PADDING = 12;
	constexpr int CORNER_RADIUS = 10;
}

Card::Card(wxWindow* _parent, const wxString& _title, bool _collapsible) : wxPanel(_parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(Theme::SURFACE);
	Bind(wxEVT_PAINT, &Card::OnPaint, this);

	const int padding = FromDIP(PADDING);

	wxStaticText* title = new wxStaticText(this, wxID_ANY, _title);
	title->SetFont(GetFont().Bold().Scaled(1.1f));
	title->SetForegroundColour(Theme::TEXT);

	wxBoxSizer* titleRow = new wxBoxSizer(wxHORIZONTAL);
	titleRow->Add(title, 1, wxALIGN_CENTER_VERTICAL);

	if (_collapsible)
	{
		toggle = new wxStaticText(this, wxID_ANY, "Hide");
		toggle->SetForegroundColour(Theme::ACCENT);
		toggle->SetFont(GetFont().Bold());
		toggle->SetCursor(wxCursor(wxCURSOR_HAND));
		titleRow->Add(toggle, 0, wxALIGN_CENTER_VERTICAL);

		// clicking the title or the link toggles the card
		toggle->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&)
			{
				SetExpanded(!expanded);
			});

		title->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&)
			{
				SetExpanded(!expanded);
			});
	}

	body = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(titleRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, padding);
	outer->Add(body, 1, wxEXPAND | wxALL, padding);
	collapsedSpacer = outer->AddSpacer(padding);
	collapsedSpacer->Show(false);
	SetSizer(outer);
}

void Card::SetExpanded(bool _expanded)
{
	expanded = _expanded;
	GetSizer()->Show(body, expanded, true);
	collapsedSpacer->Show(!expanded);

	if (toggle)
	{
		toggle->SetLabelText(expanded ? "Hide" : "Show");
	}

	// the side panel scrolls, so refit it
	wxScrolledWindow* scrolledParent = dynamic_cast<wxScrolledWindow*>(GetParent());

	if (scrolledParent)
	{
		scrolledParent->Layout();
		scrolledParent->FitInside();
	}

	Refresh();
}

void Card::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);

	// fill the corners with the parent's colour
	dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
	dc.Clear();

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));

	if (!gc)
	{
		return;
	}

	const wxSize size = GetClientSize();
	gc->SetPen(wxPen(Theme::BORDER, 1));
	gc->SetBrush(wxBrush(Theme::SURFACE));
	gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, FromDIP(CORNER_RADIUS));
}
