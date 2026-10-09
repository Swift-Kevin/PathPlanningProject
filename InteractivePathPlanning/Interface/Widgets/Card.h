#pragma once

#include <wx/panel.h>

class wxBoxSizer;
class wxSizerItem;
class wxStaticText;

// Rounded panel with a title, groups controls in the side panel and help window
class Card : public wxPanel
{
private:
	// Layout
	wxBoxSizer* body = nullptr;
	// Padding under the title while collapsed
	wxSizerItem* collapsedSpacer = nullptr;

	// Show/Hide link, only on collapsible cards
	wxStaticText* toggle = nullptr;
	bool expanded = true;

	// Draws the rounded background and border
	void OnPaint(wxPaintEvent& _event);

public:
	// Collapsible cards get a Show/Hide link
	Card(wxWindow* _parent, const wxString& _title, bool _collapsible = false);

	// Sizer for the card's controls
	wxBoxSizer* Body() const { return body; }

	// Shows or hides the card's controls
	void SetExpanded(bool _expanded);
};
