#pragma once

#include <wx/scrolwin.h>

class Card;
class ColourSwatch;
class FlatButton;
class wxCheckBox;
class wxFlexGridSizer;
class wxSlider;
class wxSpinCtrl;
class wxSpinCtrlDouble;
class wxStaticText;
enum class HeuristicType;
enum class SearchAlgorithm;
enum class ToolMode;

// Settings column on the right, MainFrame handles the control events
// The middle card changes with the active tool
class SidePanel : public wxScrolledWindow
{
private:
	// Cards that get shown and hidden
	Card* wallsCard = nullptr;
	Card* brushCard = nullptr;
	Card* annotateCard = nullptr;
	Card* coloursCard = nullptr;

	// Dropdown indexes, cast to SearchAlgorithm and HeuristicType
	int algorithmIndex = 0;
	int heuristicIndex = 0;

#pragma region Building

	// Card builders
	void BuildSearchCard(wxSizer* _column);
	void BuildWallsCard(wxSizer* _column);
	void BuildBrushCard(wxSizer* _column);
	void BuildAnnotateCard(wxSizer* _column);
	void BuildMapCard(wxSizer* _column);
	void BuildColoursCard(wxSizer* _column);

	// Makes a card and adds it to the column
	Card* AddCard(wxSizer* _column, const wxString& _title, bool _collapsible = false);
	// Normal and hint text
	wxStaticText* CreateLabel(wxWindow* _parent, const wxString& _text);
	wxStaticText* CreateHint(wxWindow* _parent, const wxString& _text);

	// Label on the left, control on the right
	void AddLabeledControl(Card* _card, const wxString& _label, wxWindow* _control);
	// Label and value, with a slider underneath
	wxSlider* AddSlider(Card* _card, const wxString& _label, wxStaticText*& _valueText, int _value, int _min, int _max);
	// Two buttons side by side
	void AddButtonPair(Card* _card, FlatButton* _left, FlatButton* _right);
	// Label and colour swatch in the colours table
	ColourSwatch* AddColourSwatch(wxFlexGridSizer* _table, wxWindow* _parent, const wxString& _label);

#pragma endregion

	// Shows _names in a menu under _button, returns the picked index or _current
	int PickFromMenu(FlatButton* _button, const wxString _names[], int _count, int _current);
	// Greys out the weight and heuristic unless A* is picked
	void UpdateSearchControls();

public:
	explicit SidePanel(wxWindow* _parent);

#pragma region Controls

	// Search
	FlatButton* algorithmButton = nullptr;
	wxSpinCtrlDouble* weightInput = nullptr;
	wxSlider* weightSlider = nullptr;
	FlatButton* heuristicButton = nullptr;
	wxCheckBox* diagonalCheck = nullptr;
	wxCheckBox* autoRunCheck = nullptr;
	wxSlider* speedSlider = nullptr;
	wxStaticText* speedValue = nullptr;

	// Cost Painter
	wxSlider* radiusSlider = nullptr;
	wxStaticText* radiusValue = nullptr;
	wxSlider* paintSpeedSlider = nullptr;
	wxStaticText* paintSpeedValue = nullptr;

	// Map size and random maps
	wxSpinCtrl* widthInput = nullptr;
	wxSpinCtrl* heightInput = nullptr;
	FlatButton* resizeButton = nullptr;
	wxSpinCtrl* seedInput = nullptr;
	FlatButton* generateButton = nullptr;
	FlatButton* newSeedButton = nullptr;

	// Colours
	ColourSwatch* startColour = nullptr;
	ColourSwatch* endColour = nullptr;
	ColourSwatch* pathColour = nullptr;
	ColourSwatch* frontierColour = nullptr;
	ColourSwatch* visitedColour = nullptr;
	ColourSwatch* wallColour = nullptr;
	ColourSwatch* highCostColour = nullptr;
	ColourSwatch* highlightColour = nullptr;
	ColourSwatch* drawingColour = nullptr;
	FlatButton* resetColoursButton = nullptr;

#pragma endregion

	// Shows the active tool's card
	void ShowToolCard(ToolMode _mode);
	// Resets every control to its default
	void RestoreDefaults();

	// Picked algorithm and heuristic
	SearchAlgorithm SelectedAlgorithm() const;
	HeuristicType SelectedHeuristic() const;
	// Show the choices menu, return true if the choice changed
	bool ChooseAlgorithm();
	bool ChooseHeuristic();

	// Heuristic weight from the number box
	float HeuristicWeight() const;
	// Keeps the weight box and slider in sync
	void SyncWeightInputToSlider();
	void SyncWeightSliderToInput();

	// Slider values in real units
	double StepsPerSecond() const;
	float PaintRate() const;

	// Updates the slider value labels
	void UpdateValueLabels();
};
