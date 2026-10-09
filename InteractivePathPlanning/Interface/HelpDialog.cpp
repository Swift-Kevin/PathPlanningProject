#pragma region Includes

#include "Interface/HelpDialog.h"

#include "Interface/Style/Theme.h"
#include "Interface/Widgets/Card.h"
#include "Interface/Widgets/FlatButton.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#pragma endregion

namespace
{
	// Spacing in DIPs
	constexpr int OUTER_GAP = 16;
	constexpr int CARD_GAP = 12;
	constexpr int ROW_GAP = 5;
	constexpr int GROUP_GAP = 12;
	constexpr int COLUMN_GAP = 18;
	// Paragraph wrap width
	constexpr int TEXT_WIDTH = 360;
	// Left column width so the groups line up
	constexpr int INPUT_COLUMN_WIDTH = 130;
}

HelpDialog::HelpDialog(wxWindow* _parent) : wxDialog(_parent, wxID_ANY, "Help")
{
	SetBackgroundColour(Theme::PANEL_BACKGROUND);

	// three cards side by side
	wxBoxSizer* columns = new wxBoxSizer(wxHORIZONTAL);
	columns->Add(BuildMouseCard(), 0, wxEXPAND | wxRIGHT, FromDIP(CARD_GAP));
	columns->Add(BuildKeyboardCard(), 0, wxEXPAND | wxRIGHT, FromDIP(CARD_GAP));
	columns->Add(BuildAlgorithmsCard(), 0, wxEXPAND);

	FlatButton* closeButton = new FlatButton(this, "Close", true);
	closeButton->SetMinSize(wxSize(FromDIP(96), -1));
	closeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			EndModal(wxID_CLOSE);
		});

	wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(columns, 1, wxEXPAND | wxALL, FromDIP(OUTER_GAP));
	outer->Add(closeButton, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(OUTER_GAP));
	SetSizerAndFit(outer);

	Bind(wxEVT_CHAR_HOOK, &HelpDialog::OnCharHook, this);
	CentreOnParent();
}

#pragma region Cards

Card* HelpDialog::BuildMouseCard()
{
	Card* card = new Card(this, "Mouse");
	const int inputWidth = FromDIP(INPUT_COLUMN_WIDTH);

	AddControlGroup(card, "Tile State", inputWidth,
	{
		{ "Click or drag", "Toggle walls" },
		{ "Drag the Start or End", "Move it" },
		{ "Right-click a cell", "Make it the start or the end" },
	});

	AddControlGroup(card, "Cost Painter", inputWidth,
	{
		{ "Hold the left button", "Raise the cost under the brush" },
		{ "Hold the right button", "Lower it" },
		{ "Mouse wheel or [ ]", "Change the brush size" },
	});

	AddControlGroup(card, "Annotate", inputWidth,
	{
		{ "Drag", "Draw over the map" },
		{ "Right-drag", "Erase drawings" },
	});

	return card;
}

Card* HelpDialog::BuildKeyboardCard()
{
	Card* card = new Card(this, "Keyboard");

	AddControlGroup(card, "Tools and searching", FromDIP(INPUT_COLUMN_WIDTH),
	{
		{ "1, 2, 3", "Tile State, Cost Painter, Annotate" },
		{ "Space", "Find the path" },
		{ "A", "Animate, pause or resume" },
		{ "Esc", "Clear the path" },
		{ "Ctrl+Z, Ctrl+Y", "Undo and redo map edits" },
	});

	AddControlGroup(card, "Moving through a search", FromDIP(INPUT_COLUMN_WIDTH),
	{
		{ "Right or N, Left", "Step forward or back" },
		{ "Page Down, Page Up", "Jump a quarter forward or back" },
		{ "Home, End", "Jump to the start or the end" },
	});

	AddControlGroup(card, "Presenting", FromDIP(INPUT_COLUMN_WIDTH),
	{
		{ "F11", "Full screen grid, Esc to exit" },
		{ "Ctrl+S", "Save the grid as a 1920 x 1080 image" },
		{ "F1", "Open this window" },
	});

	return card;
}

Card* HelpDialog::BuildAlgorithmsCard()
{
	Card* card = new Card(this, "Algorithms");

	// formula on its own line in the normal text colour
	AddParagraph(card, "A* scores each cell it reaches and expands the lowest score first.");
	wxStaticText* formula = new wxStaticText(card, wxID_ANY, "score = given cost + weight x heuristic");
	formula->SetFont(card->GetFont().Bold());
	formula->SetForegroundColour(Theme::TEXT);
	card->Body()->Add(formula, 0, wxBOTTOM, FromDIP(GROUP_GAP));
	AddParagraph(card, "Given cost is the cost to reach the cell. The heuristic is an estimate of the cost left "
		"to the end.");

	AddControlGroup(card, "Heuristic weight", 0,
	{
		{ "0", "Same as Dijkstra" },
		{ "1", "Heads for the end and still finds the cheapest path" },
		{ "Above 1", "Explores less, but the path may cost more" },
	});

	AddParagraph(card, "Dijkstra spreads out by cost. Breadth-first ignores cost and finds the path with the fewest "
		"moves, which isn't always the cheapest.");

	AddParagraph(card, "Use Octile with diagonal moves and Manhattan without them. Euclidean and Chebyshev are "
		"other ways to measure distance.");

	return card;
}

#pragma endregion

#pragma region Helpers

void HelpDialog::AddControlGroup(Card* _card, const wxString& _heading, int _inputWidth, const std::vector<ControlRow>& _rows)
{
	if (!_heading.empty())
	{
		wxStaticText* heading = new wxStaticText(_card, wxID_ANY, _heading);
		heading->SetFont(_card->GetFont().Bold());
		heading->SetForegroundColour(Theme::TEXT);
		_card->Body()->Add(heading, 0, wxBOTTOM, FromDIP(ROW_GAP));
	}

	// input on the left, action on the right
	wxFlexGridSizer* table = new wxFlexGridSizer(2, FromDIP(ROW_GAP), FromDIP(COLUMN_GAP));
	table->AddGrowableCol(1, 1);

	for (const ControlRow& row : _rows)
	{
		wxStaticText* input = new wxStaticText(_card, wxID_ANY, row.first);
		input->SetForegroundColour(Theme::TEXT);

		if (_inputWidth > 0)
		{
			input->SetMinSize(wxSize(_inputWidth, -1));
		}

		wxStaticText* action = new wxStaticText(_card, wxID_ANY, row.second);
		action->SetForegroundColour(Theme::TEXT_MUTED);

		table->Add(input);
		table->Add(action);
	}

	_card->Body()->Add(table, 0, wxEXPAND | wxBOTTOM, FromDIP(GROUP_GAP));
}

void HelpDialog::AddParagraph(Card* _card, const wxString& _text)
{
	wxStaticText* paragraph = new wxStaticText(_card, wxID_ANY, _text);
	paragraph->SetForegroundColour(Theme::TEXT_MUTED);
	paragraph->Wrap(FromDIP(TEXT_WIDTH));
	_card->Body()->Add(paragraph, 0, wxBOTTOM, FromDIP(GROUP_GAP));
}

#pragma endregion

void HelpDialog::OnCharHook(wxKeyEvent& _event)
{
	if (_event.GetKeyCode() == WXK_ESCAPE)
	{
		EndModal(wxID_CLOSE);
		return;
	}

	_event.Skip();
}
