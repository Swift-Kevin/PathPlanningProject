#pragma once

#include <wx/window.h>

// Custom drawn button that matches the toolbar, sends wxEVT_BUTTON when clicked
// Primary buttons use the accent fill, dropdown buttons show a chevron
class FlatButton : public wxWindow
{
private:
	wxString text;
	bool primary = false;
	bool dropdown = false;

	// Mouse State
	bool hovered = false;
	bool pressed = false;

	// Events
	void OnPaint(wxPaintEvent& _event);
	void OnEnter(wxMouseEvent& _event);
	void OnLeave(wxMouseEvent& _event);
	void OnLeftDown(wxMouseEvent& _event);
	void OnLeftUp(wxMouseEvent& _event);
	void OnCaptureLost(wxMouseCaptureLostEvent& _event);

protected:
	// Text size plus padding and chevron
	wxSize DoGetBestClientSize() const override;
	// Redraws greyed out when disabled
	void DoEnable(bool _enable) override;

public:
	FlatButton(wxWindow* _parent, const wxString& _text, bool _primary = false);

	// Sets the button text
	void SetText(const wxString& _text);
	// Shows a chevron for buttons that open a menu
	void SetDropdown(bool _dropdown);
};
