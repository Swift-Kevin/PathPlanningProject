#pragma region Includes

#include "Interface/SidePanel.h"

#include "Interface/GridCanvas.h"
#include "Interface/Style/Theme.h"
#include "Interface/Widgets/Card.h"
#include "Interface/Widgets/ColourSwatch.h"
#include "Interface/Widgets/FlatButton.h"
#include "Map/Grid.h"
#include "Map/MapGenerator.h"
#include "Search/PathFinder.h"

#include <wx/checkbox.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include <cmath>

#pragma endregion

namespace
{
	// Spacing in DIPs
	constexpr int CARD_GAP = 10;
	constexpr int ROW_GAP = 7;
	constexpr int PANEL_WIDTH = 360;

	// Heuristic weight, the slider moves in steps of 1 / WEIGHT_SLIDER_STEPS
	constexpr float DEFAULT_HEURISTIC_WEIGHT = 1.0f;
	constexpr float MAX_HEURISTIC_WEIGHT = 5.0f;
	constexpr int WEIGHT_SLIDER_STEPS = 20;

	// Dropdown choices, same order as SearchAlgorithm and HeuristicType
	const wxString ALGORITHM_NAMES[] = { "A*", "Dijkstra", "Breadth-first" };
	constexpr int ALGORITHM_COUNT = 3;
	const wxString HEURISTIC_NAMES[] = { "Octile", "Euclidean", "Manhattan", "Chebyshev" };
	constexpr int HEURISTIC_COUNT = 4;
	// Menu id for the first dropdown choice, the rest follow
	constexpr int FIRST_CHOICE_MENU_ID = 2000;

	// Animation speed slider is exponential, from slow to nearly instant
	constexpr int DEFAULT_SPEED_POSITION = 50;
	constexpr int MAX_SPEED_POSITION = 100;
	constexpr double SLOWEST_STEPS_PER_SECOND = 4.0;
	constexpr double SPEED_POSITIONS_PER_TENFOLD = 33.0;

	// Brush
	constexpr int DEFAULT_BRUSH_RADIUS = 2;
	constexpr int DEFAULT_PAINT_SPEED_POSITION = 8;
	constexpr int MAX_PAINT_SPEED_POSITION = 30;
	constexpr float PAINT_RATE_PER_POSITION = 0.5f;

	// Random maps
	constexpr int DEFAULT_SEED = 1;
}

SidePanel::SidePanel(wxWindow* _parent) : wxScrolledWindow(_parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
{
	SetBackgroundColour(Theme::PANEL_BACKGROUND);
	SetScrollRate(0, FromDIP(16));

	wxBoxSizer* column = new wxBoxSizer(wxVERTICAL);
	BuildSearchCard(column);
	BuildWallsCard(column);
	BuildBrushCard(column);
	BuildAnnotateCard(column);
	BuildMapCard(column);
	BuildColoursCard(column);

	wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(column, 1, wxEXPAND | wxALL, FromDIP(CARD_GAP));
	SetSizer(outer);
	SetMinSize(wxSize(FromDIP(PANEL_WIDTH), -1));

	RestoreDefaults();
}

#pragma region Cards

void SidePanel::BuildSearchCard(wxSizer* _column)
{
	Card* card = AddCard(_column, "Search");

	// algorithm and heuristic open a menu when clicked
	algorithmButton = new FlatButton(card, ALGORITHM_NAMES[0]);
	algorithmButton->SetDropdown(true);
	algorithmButton->SetMinSize(wxSize(FromDIP(130), -1));
	AddLabeledControl(card, "Algorithm", algorithmButton);

	// heuristic weight, type a value or use the slider
	weightInput = new wxSpinCtrlDouble(card, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(90, -1)),
		wxSP_ARROW_KEYS, 0.0, MAX_HEURISTIC_WEIGHT, DEFAULT_HEURISTIC_WEIGHT, 1.0 / WEIGHT_SLIDER_STEPS);
	weightInput->SetDigits(2);
	AddLabeledControl(card, "Heuristic weight", weightInput);

	weightSlider = new wxSlider(card, wxID_ANY, 0, 0, static_cast<int>(MAX_HEURISTIC_WEIGHT * WEIGHT_SLIDER_STEPS));
	card->Body()->Add(weightSlider, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP + 4));

	heuristicButton = new FlatButton(card, HEURISTIC_NAMES[0]);
	heuristicButton->SetDropdown(true);
	heuristicButton->SetMinSize(wxSize(FromDIP(130), -1));
	AddLabeledControl(card, "Heuristic", heuristicButton);

	diagonalCheck = new wxCheckBox(card, wxID_ANY, "Allow diagonal moves");
	card->Body()->Add(diagonalCheck, 0, wxBOTTOM, FromDIP(ROW_GAP));

	autoRunCheck = new wxCheckBox(card, wxID_ANY, "Re-run when the map changes");
	card->Body()->Add(autoRunCheck, 0, wxBOTTOM, FromDIP(ROW_GAP + 4));

	speedSlider = AddSlider(card, "Animation speed", speedValue, DEFAULT_SPEED_POSITION, 0, MAX_SPEED_POSITION);
}

void SidePanel::BuildWallsCard(wxSizer* _column)
{
	wallsCard = AddCard(_column, "Walls");

	const wxString hint = "Click or drag to toggle walls. Drag a marker\n"
		"to move it, or right-click a cell to place one.";
	wallsCard->Body()->Add(CreateHint(wallsCard, hint));
}

void SidePanel::BuildBrushCard(wxSizer* _column)
{
	brushCard = AddCard(_column, "Cost Painter");

	radiusSlider = AddSlider(brushCard, "Brush radius", radiusValue, DEFAULT_BRUSH_RADIUS, 0, GridCanvas::MAX_BRUSH_RADIUS);
	paintSpeedSlider = AddSlider(brushCard, "Paint speed", paintSpeedValue, DEFAULT_PAINT_SPEED_POSITION, 1, MAX_PAINT_SPEED_POSITION);

	const wxString hint = "Hold the left button to raise cost, the right to\n"
		"lower it. The search updates when you let go.";
	brushCard->Body()->Add(CreateHint(brushCard, hint));
}

void SidePanel::BuildAnnotateCard(wxSizer* _column)
{
	annotateCard = AddCard(_column, "Annotate");

	const wxString hint = "Drag to draw anywhere over the map, right-drag\n"
		"to erase. Drawings never change the map.";
	annotateCard->Body()->Add(CreateHint(annotateCard, hint));
}

void SidePanel::BuildMapCard(wxSizer* _column)
{
	Card* card = AddCard(_column, "Map");
	const wxSize inputSize = FromDIP(wxSize(58, -1));

	widthInput = new wxSpinCtrl(card, wxID_ANY, wxEmptyString, wxDefaultPosition, inputSize,
		wxSP_ARROW_KEYS, Grid::MIN_SIZE, Grid::MAX_SIZE, Grid::MIN_SIZE);
	heightInput = new wxSpinCtrl(card, wxID_ANY, wxEmptyString, wxDefaultPosition, inputSize,
		wxSP_ARROW_KEYS, Grid::MIN_SIZE, Grid::MAX_SIZE, Grid::MIN_SIZE);
	resizeButton = new FlatButton(card, "Resize");

	wxBoxSizer* sizeRow = new wxBoxSizer(wxHORIZONTAL);
	sizeRow->Add(CreateLabel(card, "Width"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
	sizeRow->Add(widthInput, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
	sizeRow->Add(CreateLabel(card, "Height"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
	sizeRow->Add(heightInput, 0, wxALIGN_CENTER_VERTICAL);
	sizeRow->AddStretchSpacer();
	sizeRow->Add(resizeButton, 0, wxALIGN_CENTER_VERTICAL);
	card->Body()->Add(sizeRow, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP + 4));

	// random maps, same seed gives the same map
	seedInput = new wxSpinCtrl(card, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(130, -1)),
		wxSP_ARROW_KEYS, 0, static_cast<int>(MapGenerator::MAX_SEED), 0);
	AddLabeledControl(card, "Random map seed", seedInput);

	generateButton = new FlatButton(card, "Generate");
	newSeedButton = new FlatButton(card, "New seed");
	AddButtonPair(card, generateButton, newSeedButton);
}

void SidePanel::BuildColoursCard(wxSizer* _column)
{
	coloursCard = AddCard(_column, "Colors", true);

	// two label and swatch pairs per row
	wxFlexGridSizer* table = new wxFlexGridSizer(4, FromDIP(6), FromDIP(8));
	table->AddGrowableCol(0, 1);
	table->AddGrowableCol(2, 1);

	startColour = AddColourSwatch(table, coloursCard, "Start");
	endColour = AddColourSwatch(table, coloursCard, "End");
	pathColour = AddColourSwatch(table, coloursCard, "Path");
	frontierColour = AddColourSwatch(table, coloursCard, "Frontier");
	visitedColour = AddColourSwatch(table, coloursCard, "Visited");
	wallColour = AddColourSwatch(table, coloursCard, "Wall");
	highCostColour = AddColourSwatch(table, coloursCard, "High cost");
	highlightColour = AddColourSwatch(table, coloursCard, "Highlight");
	drawingColour = AddColourSwatch(table, coloursCard, "Drawing");
	coloursCard->Body()->Add(table, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP));

	resetColoursButton = new FlatButton(coloursCard, "Reset colors");
	coloursCard->Body()->Add(resetColoursButton, 0, wxEXPAND);
}

#pragma endregion

#pragma region Helpers

Card* SidePanel::AddCard(wxSizer* _column, const wxString& _title, bool _collapsible)
{
	Card* card = new Card(this, _title, _collapsible);
	_column->Add(card, 0, wxEXPAND | wxBOTTOM, FromDIP(CARD_GAP));
	return card;
}

wxStaticText* SidePanel::CreateLabel(wxWindow* _parent, const wxString& _text)
{
	wxStaticText* label = new wxStaticText(_parent, wxID_ANY, _text);
	label->SetForegroundColour(Theme::TEXT);
	return label;
}

wxStaticText* SidePanel::CreateHint(wxWindow* _parent, const wxString& _text)
{
	wxStaticText* hint = new wxStaticText(_parent, wxID_ANY, _text);
	hint->SetForegroundColour(Theme::TEXT_MUTED);
	return hint;
}

void SidePanel::AddLabeledControl(Card* _card, const wxString& _label, wxWindow* _control)
{
	wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
	row->Add(CreateLabel(_card, _label), 1, wxALIGN_CENTER_VERTICAL);
	row->Add(_control, 0, wxALIGN_CENTER_VERTICAL);
	_card->Body()->Add(row, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP));
}

wxSlider* SidePanel::AddSlider(Card* _card, const wxString& _label, wxStaticText*& _valueText, int _value, int _min, int _max)
{
	_valueText = new wxStaticText(_card, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
	_valueText->SetForegroundColour(Theme::ACCENT);
	_valueText->SetFont(GetFont().Bold());
	_valueText->SetMinSize(FromDIP(wxSize(150, -1)));

	wxBoxSizer* labelRow = new wxBoxSizer(wxHORIZONTAL);
	labelRow->Add(CreateLabel(_card, _label), 1, wxALIGN_CENTER_VERTICAL);
	labelRow->Add(_valueText, 0, wxALIGN_CENTER_VERTICAL);

	wxSlider* slider = new wxSlider(_card, wxID_ANY, _value, _min, _max);
	_card->Body()->Add(labelRow, 0, wxEXPAND);
	_card->Body()->Add(slider, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP));
	return slider;
}

void SidePanel::AddButtonPair(Card* _card, FlatButton* _left, FlatButton* _right)
{
	wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
	row->Add(_left, 1, wxRIGHT, FromDIP(ROW_GAP));
	row->Add(_right, 1);
	_card->Body()->Add(row, 0, wxEXPAND);
}

ColourSwatch* SidePanel::AddColourSwatch(wxFlexGridSizer* _table, wxWindow* _parent, const wxString& _label)
{
	ColourSwatch* swatch = new ColourSwatch(_parent);
	_table->Add(CreateLabel(_parent, _label), 0, wxALIGN_CENTER_VERTICAL);
	_table->Add(swatch, 0, wxALIGN_CENTER_VERTICAL);
	return swatch;
}

#pragma endregion

#pragma region State

void SidePanel::ShowToolCard(ToolMode _mode)
{
	wallsCard->Show(_mode == ToolMode::TileState);
	brushCard->Show(_mode == ToolMode::CostPaint);
	annotateCard->Show(_mode == ToolMode::Annotate);
	Layout();
	FitInside();
}

void SidePanel::RestoreDefaults()
{
	algorithmIndex = static_cast<int>(SearchAlgorithm::AStar);
	algorithmButton->SetText(ALGORITHM_NAMES[algorithmIndex]);
	weightInput->SetValue(DEFAULT_HEURISTIC_WEIGHT);
	SyncWeightSliderToInput();
	heuristicIndex = static_cast<int>(HeuristicType::Octile);
	heuristicButton->SetText(HEURISTIC_NAMES[heuristicIndex]);
	UpdateSearchControls();
	diagonalCheck->SetValue(true);
	autoRunCheck->SetValue(true);
	speedSlider->SetValue(DEFAULT_SPEED_POSITION);

	radiusSlider->SetValue(DEFAULT_BRUSH_RADIUS);
	paintSpeedSlider->SetValue(DEFAULT_PAINT_SPEED_POSITION);
	seedInput->SetValue(DEFAULT_SEED);

	coloursCard->SetExpanded(false);
	UpdateValueLabels();
}

SearchAlgorithm SidePanel::SelectedAlgorithm() const
{
	return static_cast<SearchAlgorithm>(algorithmIndex);
}

HeuristicType SidePanel::SelectedHeuristic() const
{
	return static_cast<HeuristicType>(heuristicIndex);
}

bool SidePanel::ChooseAlgorithm()
{
	const int picked = PickFromMenu(algorithmButton, ALGORITHM_NAMES, ALGORITHM_COUNT, algorithmIndex);

	if (picked == algorithmIndex)
	{
		return false;
	}

	algorithmIndex = picked;
	algorithmButton->SetText(ALGORITHM_NAMES[algorithmIndex]);
	UpdateSearchControls();
	return true;
}

bool SidePanel::ChooseHeuristic()
{
	const int picked = PickFromMenu(heuristicButton, HEURISTIC_NAMES, HEURISTIC_COUNT, heuristicIndex);

	if (picked == heuristicIndex)
	{
		return false;
	}

	heuristicIndex = picked;
	heuristicButton->SetText(HEURISTIC_NAMES[heuristicIndex]);
	return true;
}

int SidePanel::PickFromMenu(FlatButton* _button, const wxString _names[], int _count, int _current)
{
	wxMenu menu;

	for (int i = 0; i < _count; ++i)
	{
		menu.AppendRadioItem(FIRST_CHOICE_MENU_ID + i, _names[i])->Check(i == _current);
	}

	// open under the button, closing the menu keeps the current choice
	const int pickedId = _button->GetPopupMenuSelectionFromUser(menu, wxPoint(0, _button->GetSize().y));
	return pickedId == wxID_NONE ? _current : pickedId - FIRST_CHOICE_MENU_ID;
}

void SidePanel::UpdateSearchControls()
{
	// only A* uses the heuristic
	const bool usesHeuristic = SelectedAlgorithm() == SearchAlgorithm::AStar;
	weightInput->Enable(usesHeuristic);
	weightSlider->Enable(usesHeuristic);
	heuristicButton->Enable(usesHeuristic);
}

float SidePanel::HeuristicWeight() const
{
	return static_cast<float>(weightInput->GetValue());
}

void SidePanel::SyncWeightInputToSlider()
{
	weightInput->SetValue(static_cast<double>(weightSlider->GetValue()) / WEIGHT_SLIDER_STEPS);
}

void SidePanel::SyncWeightSliderToInput()
{
	weightSlider->SetValue(static_cast<int>(std::lround(weightInput->GetValue() * WEIGHT_SLIDER_STEPS)));
}

double SidePanel::StepsPerSecond() const
{
	return SLOWEST_STEPS_PER_SECOND * std::pow(10.0, speedSlider->GetValue() / SPEED_POSITIONS_PER_TENFOLD);
}

float SidePanel::PaintRate() const
{
	return paintSpeedSlider->GetValue() * PAINT_RATE_PER_POSITION;
}

void SidePanel::UpdateValueLabels()
{
	speedValue->SetLabelText(wxString::Format("%.0f steps per second", StepsPerSecond()));
	paintSpeedValue->SetLabelText(wxString::Format("%.1f cost per second", PaintRate()));

	const int radius = radiusSlider->GetValue();
	radiusValue->SetLabelText(radius == 0 ? wxString("single cell") : wxString::Format("%d cells", radius));
}

#pragma endregion
