#pragma once

#include <wx/window.h>

// Colour box that opens the colour picker when clicked
// Sends wxEVT_COLOURPICKER_CHANGED when the colour changes
class ColourSwatch : public wxWindow
{
private:
	wxColour colour;
	bool hovered = false;

	// Events
	void OnPaint(wxPaintEvent& _event);
	void OnEnter(wxMouseEvent& _event);
	void OnLeave(wxMouseEvent& _event);
	void OnLeftDown(wxMouseEvent& _event);

public:
	explicit ColourSwatch(wxWindow* _parent);

	const wxColour& GetColour() const { return colour; }
	void SetColour(const wxColour& _colour);
};
