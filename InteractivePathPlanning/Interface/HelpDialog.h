#pragma once

#include <wx/dialog.h>

#include <utility>
#include <vector>

class Card;

// Help window with the controls and algorithm notes
class HelpDialog : public wxDialog
{
private:
	// Input and what it does
	using ControlRow = std::pair<wxString, wxString>;

#pragma region Building

	// Card builders
	Card* BuildMouseCard();
	Card* BuildKeyboardCard();
	Card* BuildAlgorithmsCard();

	// Heading with a two column list of controls, _inputWidth is the left column's min width
	void AddControlGroup(Card* _card, const wxString& _heading, int _inputWidth, const std::vector<ControlRow>& _rows);
	// Wrapped paragraph of hint text
	void AddParagraph(Card* _card, const wxString& _text);

#pragma endregion

	// Closes on Escape
	void OnCharHook(wxKeyEvent& _event);

public:
	explicit HelpDialog(wxWindow* _parent);
};
