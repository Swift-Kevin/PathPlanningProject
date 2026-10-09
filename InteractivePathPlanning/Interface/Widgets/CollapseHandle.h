#pragma once

#include <wx/window.h>

// Strip on the side panel's left edge that opens and closes the panel
// Sends wxEVT_BUTTON when clicked
class CollapseHandle : public wxWindow
{
private:
	// Panel is closed
	bool collapsed = false;
	bool hovered = false;

	// Events
	void OnPaint(wxPaintEvent& _event);
	void OnEnter(wxMouseEvent& _event);
	void OnLeave(wxMouseEvent& _event);
	void OnLeftDown(wxMouseEvent& _event);

public:
	explicit CollapseHandle(wxWindow* _parent);

	// Updates the width and chevron direction
	void SetCollapsed(bool _collapsed);
	bool IsCollapsed() const { return collapsed; }
};
