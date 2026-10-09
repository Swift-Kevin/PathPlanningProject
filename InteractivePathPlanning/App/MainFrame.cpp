#pragma region Includes

#include "App/MainFrame.h"

#include "Interface/HelpDialog.h"
#include "Interface/SidePanel.h"
#include "Interface/Style/Icons.h"
#include "Interface/Widgets/CollapseHandle.h"
#include "Interface/Widgets/ColourSwatch.h"
#include "Interface/Widgets/FlatButton.h"
#include "Interface/Widgets/FlatToolBar.h"
#include "Map/MapGenerator.h"
#include "Map/TestMaps.h"

#include <wx/accel.h>
#include <wx/checkbox.h>
#include <wx/clrpicker.h>
#include <wx/display.h>
#include <wx/filedlg.h>
#include <wx/iconbndl.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <vector>

#pragma endregion

namespace
{
	// Toolbar item ids
	enum ToolBarId
	{
		ID_TILE_STATE = wxID_HIGHEST + 100,
		ID_COST_PAINTER,
		ID_ANNOTATE,
		ID_MAP,
		ID_VIEW,
		ID_FIND_PATH,
		ID_ANIMATE,
		ID_STEP,
		ID_STEP_MENU,
		ID_CLEAR,
		ID_RESET,
		ID_HELP,
	};

	// Keyboard shortcut ids
	enum ShortcutId
	{
		ID_UNDO = wxID_HIGHEST + 200,
		ID_REDO,
		ID_FULL_SCREEN,
		ID_SAVE_IMAGE,
	};

	// Items in the View and Clear menus
	enum MenuItemId
	{
		ID_SHOW_MAP = wxID_HIGHEST + 300,
		ID_SHOW_COSTS,
		ID_SHOW_COST_NUMBERS,
		ID_SHOW_VISITED,
		ID_SHOW_FRONTIER,
		ID_SHOW_WALLS,
		ID_SHOW_PATH,
		ID_SHOW_DRAWINGS,
		ID_SHOW_EVERYTHING,
		ID_CLEAR_WALLS,
		ID_CLEAR_COSTS,
		ID_CLEAR_PATH,
		ID_CLEAR_DRAWINGS,
	};

	// Step menu choices, choice i uses menu id FIRST_STEP_MENU_ID + i
	constexpr int STEP_INTERVALS[] = { 1, 5, 10, 25, 50, 100 };
	constexpr int FIRST_STEP_MENU_ID = wxID_HIGHEST + 400;

	// Animation tick, about 60 fps
	constexpr int ANIMATION_FRAME_MS = 15;

	// Window size in DIPs
	constexpr int PREFERRED_WINDOW_WIDTH = 1440;
	constexpr int PREFERRED_WINDOW_HEIGHT = 880;
	constexpr int MIN_WINDOW_WIDTH = 1200;
	constexpr int MIN_WINDOW_HEIGHT = 680;
	constexpr int STATUS_STRIP_HEIGHT = 30;

	// Saved image size
	constexpr int IMAGE_WIDTH = 1920;
	constexpr int IMAGE_HEIGHT = 1080;

	// Only sets the label if the text changed
	void SetLabelIfChanged(wxStaticText* _label, const wxString& _text)
	{
		if (_label->GetLabelText() != _text)
		{
			_label->SetLabelText(_text);
		}
	}
}

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "Interactive Path Planning")
, grid(Grid::MIN_SIZE, Grid::MIN_SIZE), animationTimer(this)
{
	SetIcons(Icons::AppIcon());
	SetBackgroundColour(Theme::PANEL_BACKGROUND);

	BuildLayout();
	BindEvents();
	LoadFactoryState();

	// open at the preferred size, capped to the screen
	const wxRect screen = wxDisplay(this).GetClientArea();
	const wxSize preferred = FromDIP(wxSize(PREFERRED_WINDOW_WIDTH, PREFERRED_WINDOW_HEIGHT));
	SetSize(std::min(preferred.x, screen.width), std::min(preferred.y, screen.height));

	// at least as wide as the toolbar needs
	const int minimumWidth = std::max(FromDIP(MIN_WINDOW_WIDTH), toolBar->MinimumWidth());
	SetMinClientSize(wxSize(minimumWidth, FromDIP(MIN_WINDOW_HEIGHT)));
	Centre();
}

#pragma region Setup

void MainFrame::BuildLayout()
{
	BuildToolBar();
	canvas = new GridCanvas(this, grid, palette);
	sideHandle = new CollapseHandle(this);
	side = new SidePanel(this);
	BuildStatusStrip();

	// the handle stays when the panel is closed
	wxBoxSizer* middle = new wxBoxSizer(wxHORIZONTAL);
	middle->Add(canvas, 1, wxEXPAND);
	middle->Add(sideHandle, 0, wxEXPAND);
	middle->Add(side, 0, wxEXPAND);

	wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
	root->Add(toolBar, 0, wxEXPAND);
	root->Add(middle, 1, wxEXPAND);
	root->Add(statusDivider, 0, wxEXPAND);
	root->Add(statusStrip, 0, wxEXPAND);
	SetSizer(root);
}

void MainFrame::BuildToolBar()
{
	toolBar = new FlatToolBar(this);
	toolBar->SetTitle("Path Planner");

	// tools
	toolBar->AddRadio(ID_TILE_STATE, "Tile State", Icons::TILE_STATE);
	toolBar->AddRadio(ID_COST_PAINTER, "Cost Painter", Icons::COST_PAINTER);
	toolBar->AddRadio(ID_ANNOTATE, "Annotate", Icons::ANNOTATE);
	toolBar->AddSeparator();

	// built-in maps
	std::vector<wxString> mapNames;

	for (const TestMap& map : TestMaps::All())
	{
		mapNames.push_back(map.name);
	}

	toolBar->AddDropdown(ID_MAP, Icons::MAP, mapNames);

	// layer checklist
	toolBar->AddMenuButton(ID_VIEW, "View", Icons::VIEW);

	// search controls on the right, Animate and Step keep room for their longest labels
	toolBar->AddStretch();
	toolBar->AddButton(ID_FIND_PATH, "Find Path", Icons::FIND_PATH, true);
	toolBar->AddButton(ID_ANIMATE, "Animate", Icons::PLAY);
	toolBar->ReserveLabel(ID_ANIMATE, "Resume");
	toolBar->ReserveLabel(ID_ANIMATE, "Pause");
	toolBar->AddSplitButton(ID_STEP, ID_STEP_MENU, "Step", Icons::STEP);
	toolBar->ReserveLabel(ID_STEP, wxString::Format("Step x%d", STEP_INTERVALS[std::size(STEP_INTERVALS) - 1]));
	toolBar->AddMenuButton(ID_CLEAR, "Clear", Icons::CLEAR);
	toolBar->AddSeparator();
	toolBar->AddButton(ID_RESET, "Reset", Icons::RESET);
	toolBar->AddButton(ID_HELP, wxString(), Icons::HELP);
}

void MainFrame::BuildStatusStrip()
{
	// divider line above the strip
	statusDivider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(1, 1));
	statusDivider->SetBackgroundColour(Theme::BORDER);

	statusStrip = new wxPanel(this);
	statusStrip->SetBackgroundColour(Theme::SURFACE);
	statusStrip->SetMinSize(wxSize(-1, FromDIP(STATUS_STRIP_HEIGHT)));
	statusText = new wxStaticText(statusStrip, wxID_ANY, wxString());
	statusText->SetForegroundColour(Theme::TEXT);

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(statusText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(16));
	statusStrip->SetSizer(sizer);
}

void MainFrame::BindEvents()
{
	Bind(wxEVT_BUTTON, &MainFrame::OnToolBarButton, this, ID_TILE_STATE, ID_HELP);
	Bind(wxEVT_CHOICE, &MainFrame::OnMapChosen, this, ID_MAP);
	Bind(wxEVT_TIMER, &MainFrame::OnAnimationTick, this, animationTimer.GetId());
	Bind(wxEVT_IDLE, &MainFrame::OnIdle, this);

	sideHandle->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			SetSidePanelOpen(!side->IsShown());
			canvas->SetFocus();
		});

	BindCanvas();
	BindSearchControls();
	BindToolControls();
	BindGridControls();
	BindColourControls();
	BindKeyboardShortcuts();
}

void MainFrame::BindCanvas()
{
	canvas->onEditStarting = [this]()
		{
			history.BeginChange(grid);
		};

	canvas->onGridEdited = [this](EditPhase _phase)
		{
			MapEdited(_phase);
		};

	canvas->onKey = [this](const wxKeyEvent& _event)
		{
			return OnCanvasKey(_event);
		};

	// keep the slider in sync when the wheel changes the brush
	canvas->onBrushRadiusChanged = [this](int _radius)
		{
			side->radiusSlider->SetValue(_radius);
			side->UpdateValueLabels();
		};
}

void MainFrame::BindSearchControls()
{
	// weight slider and number box update each other
	side->weightSlider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&)
		{
			side->SyncWeightInputToSlider();
			SettingsChanged();
		});

	side->weightInput->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&)
		{
			side->SyncWeightSliderToInput();
			SettingsChanged();
		});

	side->algorithmButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			if (side->ChooseAlgorithm())
			{
				SettingsChanged();
			}
		});

	side->heuristicButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			if (side->ChooseHeuristic())
			{
				SettingsChanged();
			}
		});

	side->diagonalCheck->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&)
		{
			SettingsChanged();
		});

	side->speedSlider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&)
		{
			side->UpdateValueLabels();
		});

	side->autoRunCheck->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&)
		{
			if (side->autoRunCheck->GetValue())
			{
				RunSearch(false);
			}
		});
}

void MainFrame::BindToolControls()
{
	side->radiusSlider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&)
		{
			canvas->SetBrushRadius(side->radiusSlider->GetValue());
			side->UpdateValueLabels();
		});

	side->paintSpeedSlider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&)
		{
			canvas->SetPaintRate(side->PaintRate());
			side->UpdateValueLabels();
		});
}

void MainFrame::BindGridControls()
{
	side->resizeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			ChangeGrid([this] { grid.Resize(side->widthInput->GetValue(), side->heightInput->GetValue()); });
		});

	// Generate uses the seed box, New seed picks one from the clock
	side->generateButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			GenerateMap(static_cast<std::uint32_t>(side->seedInput->GetValue()));
		});

	side->newSeedButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			const std::uint32_t seed = MapGenerator::SeedFromClock();
			side->seedInput->SetValue(static_cast<int>(seed));
			GenerateMap(seed);
		});
}

void MainFrame::BindColourControls()
{
	const std::vector<ColourSwatch*> swatches =
	{
		side->startColour, side->endColour, side->pathColour, side->frontierColour, side->visitedColour,
		side->wallColour, side->highCostColour, side->highlightColour, side->drawingColour,
	};

	for (ColourSwatch* swatch : swatches)
	{
		swatch->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent&)
			{
				ReadColourPickers();
				canvas->RefreshCells();
			});
	}

	side->resetColoursButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
		{
			palette = Palette();
			SyncColourPickers();
			canvas->RefreshCells();
		});
}

void MainFrame::BindKeyboardShortcuts()
{
	// displaying keybinds in help menu
	wxAcceleratorEntry shortcuts[6];
	shortcuts[0].Set(wxACCEL_CTRL, 'Z', ID_UNDO);
	shortcuts[1].Set(wxACCEL_CTRL, 'Y', ID_REDO);
	shortcuts[2].Set(wxACCEL_CTRL | wxACCEL_SHIFT, 'Z', ID_REDO);
	shortcuts[3].Set(wxACCEL_NORMAL, WXK_F1, ID_HELP);
	shortcuts[4].Set(wxACCEL_NORMAL, WXK_F11, ID_FULL_SCREEN);
	shortcuts[5].Set(wxACCEL_CTRL, 'S', ID_SAVE_IMAGE);
	SetAcceleratorTable(wxAcceleratorTable(6, shortcuts));

	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Undo(); }, ID_UNDO);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Redo(); }, ID_REDO);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { SetFullScreen(!IsFullScreen()); }, ID_FULL_SCREEN);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { SaveImage(); }, ID_SAVE_IMAGE);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { ShowHelp(); }, ID_HELP);
}

void MainFrame::LoadFactoryState()
{
	palette = Palette();
	side->RestoreDefaults();
	SyncColourPickers();

	layers = VisibleLayers();
	canvas->SetVisibleLayers(layers);
	canvas->ClearDrawings();
	SetSidePanelOpen(true);

	stepInterval = 1;
	UpdateStepLabel();

	canvas->SetBrushRadius(side->radiusSlider->GetValue());
	canvas->SetPaintRate(side->PaintRate());
	SetMode(ToolMode::TileState);

	// load the first map with nothing to undo
	LoadMap(0);
	history.Clear();
}

void MainFrame::ConfirmFactoryReset()
{
	wxMessageDialog confirm(this, "Resets the map, search settings, drawings and colors.", "Reset everything?",
		wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	confirm.SetYesNoLabels("Reset", "Cancel");

	if (confirm.ShowModal() == wxID_YES)
	{
		LoadFactoryState();
	}
}

void MainFrame::LoadMap(int _index)
{
	const std::vector<TestMap>& maps = TestMaps::All();

	if (_index < 0 || _index >= static_cast<int>(maps.size()))
	{
		return;
	}

	ClearSearch();
	ChangeGrid([&] { TestMaps::Load(maps[_index], grid); });
	toolBar->SetDropdownSelection(ID_MAP, _index);
}

void MainFrame::GenerateMap(std::uint32_t _seed)
{
	const int width = side->widthInput->GetValue();
	const int height = side->heightInput->GetValue();

	ClearSearch();
	ChangeGrid([&] { MapGenerator::Generate(grid, width, height, _seed); });
	toolBar->SetDropdownLabel(ID_MAP, "Random map");
}

#pragma endregion

#pragma region Toolbar Menus

void MainFrame::ShowViewMenu()
{
	wxMenu menu;
	menu.AppendCheckItem(ID_SHOW_MAP, "Map")->Check(layers.map);
	menu.AppendCheckItem(ID_SHOW_COSTS, "Cell costs")->Check(layers.costs);
	menu.AppendCheckItem(ID_SHOW_COST_NUMBERS, "Cost numbers")->Check(layers.costNumbers);
	menu.AppendCheckItem(ID_SHOW_VISITED, "Visited cells")->Check(layers.visited);
	menu.AppendCheckItem(ID_SHOW_FRONTIER, "Frontier cells")->Check(layers.frontier);
	menu.AppendCheckItem(ID_SHOW_WALLS, "Walls")->Check(layers.walls);
	menu.AppendCheckItem(ID_SHOW_PATH, "Path")->Check(layers.path);
	menu.AppendCheckItem(ID_SHOW_DRAWINGS, "Drawings")->Check(layers.drawings);
	menu.AppendSeparator();
	menu.Append(ID_SHOW_EVERYTHING, "Show everything");
	menu.AppendSeparator();
	menu.Append(ID_FULL_SCREEN, "Full screen\tF11");
	menu.Append(ID_SAVE_IMAGE, "Save image...\tCtrl+S");

	// toggle selected items
	switch (toolBar->PopupMenuBelow(ID_VIEW, menu))
	{
	case ID_FULL_SCREEN:
		SetFullScreen(true);
		return;
	case ID_SAVE_IMAGE:
		SaveImage();
		return;
	case ID_SHOW_MAP:
		layers.map = !layers.map;
		break;
	case ID_SHOW_COSTS:
		layers.costs = !layers.costs;
		break;
	case ID_SHOW_COST_NUMBERS:
		layers.costNumbers = !layers.costNumbers;
		break;
	case ID_SHOW_VISITED:
		layers.visited = !layers.visited;
		break;
	case ID_SHOW_FRONTIER:
		layers.frontier = !layers.frontier;
		break;
	case ID_SHOW_WALLS:
		layers.walls = !layers.walls;
		break;
	case ID_SHOW_PATH:
		layers.path = !layers.path;
		break;
	case ID_SHOW_DRAWINGS:
		layers.drawings = !layers.drawings;
		break;
	case ID_SHOW_EVERYTHING:
		layers = VisibleLayers();
		layers.costNumbers = true;
		break;
	default:
		return;
	}

	canvas->SetVisibleLayers(layers);
}

void MainFrame::ShowClearMenu()
{
	// grey out items that would do nothing
	wxMenu menu;
	menu.Append(ID_CLEAR_WALLS, "Clear walls")->Enable(grid.HasWalls());
	menu.Append(ID_CLEAR_COSTS, "Clear tile costs")->Enable(grid.HasRaisedCosts());
	menu.AppendSeparator();
	menu.Append(ID_CLEAR_PATH, "Clear path")->Enable(playback.HasResult());
	menu.Append(ID_CLEAR_DRAWINGS, "Clear drawings")->Enable(canvas->HasDrawings());

	switch (toolBar->PopupMenuBelow(ID_CLEAR, menu))
	{
	case ID_CLEAR_WALLS:
		ChangeGrid([this] { grid.ClearWalls(); });
		break;
	case ID_CLEAR_COSTS:
		ChangeGrid([this] { grid.ResetCosts(); });
		break;
	case ID_CLEAR_PATH:
		ClearSearch();
		break;
	case ID_CLEAR_DRAWINGS:
		canvas->ClearDrawings();
		break;
	default:
		break;
	}
}

void MainFrame::ShowStepMenu()
{
	const int choiceCount = static_cast<int>(std::size(STEP_INTERVALS));
	wxMenu menu;

	for (int i = 0; i < choiceCount; ++i)
	{
		const int interval = STEP_INTERVALS[i];
		const wxString label = interval == 1 ? wxString("1 step at a time") : wxString::Format("%d steps at a time", interval);
		menu.AppendRadioItem(FIRST_STEP_MENU_ID + i, label)->Check(interval == stepInterval);
	}

	const int picked = toolBar->PopupMenuBelow(ID_STEP, menu) - FIRST_STEP_MENU_ID;

	if (picked < 0 || picked >= choiceCount)
	{
		return;
	}

	stepInterval = STEP_INTERVALS[picked];
	UpdateStepLabel();
}

void MainFrame::ShowHelp()
{
	HelpDialog help(this);
	help.ShowModal();
	canvas->SetFocus();
}

void MainFrame::SaveImage()
{
	wxFileDialog dialog(this, "Save the grid as an image", wxEmptyString, "Path Planner.png", "PNG image (*.png)|*.png",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_OK)
	{
		// same as the canvas, at full HD
		const wxImage image = canvas->RenderImage(wxSize(IMAGE_WIDTH, IMAGE_HEIGHT)).ConvertToImage();

		if (!image.SaveFile(dialog.GetPath(), wxBITMAP_TYPE_PNG))
		{
			wxMessageBox("Couldn't save the image.", "Save image", wxOK | wxICON_WARNING, this);
		}
	}

	canvas->SetFocus();
}

#pragma endregion

#pragma region Searching

void MainFrame::ReadSettings()
{
	settings.algorithm = side->SelectedAlgorithm();
	settings.heuristicWeight = side->HeuristicWeight();
	settings.heuristic = side->SelectedHeuristic();
	settings.allowDiagonal = side->diagonalCheck->GetValue();
}

void MainFrame::RunSearch(bool _animate)
{
	ReadSettings();
	searchRerunPending = false;
	playback.Run(grid, settings, _animate);

	_animate ? StartAnimationTimer() : animationTimer.Stop();

	ShowSearch();
}

void MainFrame::ToggleAnimation()
{
	if (playback.IsPlaying())
	{
		playback.Pause();
		animationTimer.Stop();
		ShowSearch();
		return;
	}

	// resume if paused and the map hasn't changed
	if (playback.IsPaused() && playback.MatchesGrid(grid))
	{
		playback.Play();
		StartAnimationTimer();
		ShowSearch();
		return;
	}

	RunSearch(true);
}

void MainFrame::StepOnce()
{
	ReadSettings();
	animationTimer.Stop();
	playback.StepForward(grid, settings, stepInterval);
	ShowSearch();
}

void MainFrame::ShowStep(int _step)
{
	ReadSettings();
	animationTimer.Stop();
	playback.ShowStep(grid, settings, _step);
	ShowSearch();
}

void MainFrame::MoveByQuarters(int _quarters)
{
	ReadSettings();
	animationTimer.Stop();
	playback.MoveByQuarters(grid, settings, _quarters);
	ShowSearch();
}

void MainFrame::ClearSearch()
{
	animationTimer.Stop();
	playback.Clear();
	searchRerunPending = false;
	ShowSearch();
}

void MainFrame::StartAnimationTimer()
{
	lastAnimationTick = Clock::now();
	animationTimer.Start(ANIMATION_FRAME_MS);
}

void MainFrame::ShowSearch()
{
	canvas->SetSearch(playback.Result(), playback.DisplayStep());
	UpdateToolBar();
	UpdateStats();
}

#pragma endregion

#pragma region Reacting To Changes

void MainFrame::MapEdited(EditPhase _phase)
{
	// finished changes become undo steps
	if (_phase == EditPhase::Finished)
	{
		history.CommitChange();
	}

	// with re-run off, clear the old search
	if (!side->autoRunCheck->GetValue())
	{
		if (playback.HasResult())
		{
			ClearSearch();
		}

		return;
	}

	// search again once the stroke is finished
	if (_phase == EditPhase::Finished)
	{
		searchRerunPending = true;
	}
}

void MainFrame::GridChangedOutsideCanvas()
{
	canvas->RefreshCells();
	SyncSizeInputs();
	MapEdited(EditPhase::Finished);
}

void MainFrame::ChangeGrid(const std::function<void()>& _change)
{
	history.BeginChange(grid);
	_change();
	GridChangedOutsideCanvas();
}

void MainFrame::Undo()
{
	if (history.Undo(grid))
	{
		GridChangedOutsideCanvas();
	}
}

void MainFrame::Redo()
{
	if (history.Redo(grid))
	{
		GridChangedOutsideCanvas();
	}
}

void MainFrame::SettingsChanged()
{
	ReadSettings();

	if (playback.HasResult() || side->autoRunCheck->GetValue())
	{
		searchRerunPending = true;
	}
}

#pragma endregion

#pragma region UI Sync

void MainFrame::SetMode(ToolMode _mode)
{
	canvas->SetMode(_mode);
	side->ShowToolCard(_mode);

	switch (_mode)
	{
	case ToolMode::TileState:
		toolBar->SetChecked(ID_TILE_STATE);
		break;
	case ToolMode::CostPaint:
		toolBar->SetChecked(ID_COST_PAINTER);
		break;
	case ToolMode::Annotate:
		toolBar->SetChecked(ID_ANNOTATE);
		break;
	}
}

void MainFrame::SetSidePanelOpen(bool _open)
{
	sidePanelOpen = _open;
	side->Show(_open);
	sideHandle->SetCollapsed(!_open);
	Layout();
}

void MainFrame::SetFullScreen(bool _fullScreen)
{
	// hide everything but the grid, the side panel comes back how it was
	toolBar->Show(!_fullScreen);
	sideHandle->Show(!_fullScreen);
	side->Show(!_fullScreen && sidePanelOpen);
	statusDivider->Show(!_fullScreen);
	statusStrip->Show(!_fullScreen);

	ShowFullScreen(_fullScreen);
	Layout();
	canvas->SetFocus();
}

void MainFrame::UpdateToolBar()
{
	if (playback.IsPlaying())
	{
		toolBar->SetItemContent(ID_ANIMATE, "Pause", Icons::PAUSE);
	}
	else if (playback.IsPaused() && playback.MatchesGrid(grid))
	{
		toolBar->SetItemContent(ID_ANIMATE, "Resume", Icons::PLAY);
	}
	else
	{
		toolBar->SetItemContent(ID_ANIMATE, "Animate", Icons::PLAY);
	}
}

void MainFrame::UpdateStepLabel()
{
	const wxString label = stepInterval == 1 ? wxString("Step") : wxString::Format("Step x%d", stepInterval);
	toolBar->SetItemContent(ID_STEP, label, Icons::STEP);
}

void MainFrame::UpdateStats()
{
	wxString text;

	if (!playback.HasResult())
	{
		text = "Press Find Path or Animate to search.";
	}
	else if (!playback.IsFinished())
	{
		const wxString state = playback.IsPlaying() ? "Searching" : "Paused at";
		text = wxString::Format("%s step %d of %d", state, playback.DisplayStep(), playback.Result()->TotalSteps());
	}
	else if (playback.Result()->found)
	{
		text = wxString::Format("Path cost %.2f, %d nodes explored", playback.Result()->pathCost, playback.Result()->TotalSteps());
	}
	else
	{
		text = wxString::Format("No path to the end, %d nodes explored", playback.Result()->TotalSteps());
	}

	SetLabelIfChanged(statusText, text);
}

void MainFrame::SyncSizeInputs()
{
	side->widthInput->SetValue(grid.Width());
	side->heightInput->SetValue(grid.Height());
}

void MainFrame::SyncColourPickers()
{
	side->startColour->SetColour(palette.start);
	side->endColour->SetColour(palette.end);
	side->pathColour->SetColour(palette.path);
	side->frontierColour->SetColour(palette.frontier);
	side->visitedColour->SetColour(palette.visited);
	side->wallColour->SetColour(palette.wall);
	side->highCostColour->SetColour(palette.highCost);
	side->highlightColour->SetColour(palette.highlight);
	side->drawingColour->SetColour(palette.drawing);
}

void MainFrame::ReadColourPickers()
{
	palette.start = side->startColour->GetColour();
	palette.end = side->endColour->GetColour();
	palette.path = side->pathColour->GetColour();
	palette.frontier = side->frontierColour->GetColour();
	palette.visited = side->visitedColour->GetColour();
	palette.wall = side->wallColour->GetColour();
	palette.highCost = side->highCostColour->GetColour();
	palette.highlight = side->highlightColour->GetColour();
	palette.drawing = side->drawingColour->GetColour();
}

#pragma endregion

#pragma region Events

void MainFrame::OnToolBarButton(wxCommandEvent& _event)
{
	switch (_event.GetId())
	{
	case ID_TILE_STATE:
		SetMode(ToolMode::TileState);
		break;
	case ID_COST_PAINTER:
		SetMode(ToolMode::CostPaint);
		break;
	case ID_ANNOTATE:
		SetMode(ToolMode::Annotate);
		break;
	case ID_VIEW:
		ShowViewMenu();
		break;
	case ID_FIND_PATH:
		RunSearch(false);
		break;
	case ID_ANIMATE:
		ToggleAnimation();
		break;
	case ID_STEP:
		StepOnce();
		break;
	case ID_STEP_MENU:
		ShowStepMenu();
		break;
	case ID_CLEAR:
		ShowClearMenu();
		break;
	case ID_RESET:
		ConfirmFactoryReset();
		break;
	case ID_HELP:
		ShowHelp();
		break;
	default:
		_event.Skip();
		return;
	}

	// focus canvas for kbm
	canvas->SetFocus();
}

void MainFrame::OnMapChosen(wxCommandEvent& _event)
{
	LoadMap(_event.GetInt());
	canvas->SetFocus();
}

void MainFrame::OnAnimationTick(wxTimerEvent&)
{
	// delta time calc
	const Clock::time_point now = Clock::now();
	const double seconds = std::chrono::duration<double>(now - lastAnimationTick).count();
	lastAnimationTick = now;

	const bool stepChanged = playback.Advance(seconds, side->StepsPerSecond());

	if (!playback.IsPlaying())
	{
		animationTimer.Stop();
	}

	if (stepChanged || !playback.IsPlaying())
	{
		ShowSearch();
	}
}

void MainFrame::OnIdle(wxIdleEvent& _event)
{
	if (searchRerunPending)
	{
		RunSearch(false);
	}

	_event.Skip();
}

bool MainFrame::OnCanvasKey(const wxKeyEvent& _event)
{
	if (_event.HasAnyModifiers())
	{
		return false;
	}

	switch (_event.GetKeyCode())
	{
	case '1':
		SetMode(ToolMode::TileState);
		return true;
	case '2':
		SetMode(ToolMode::CostPaint);
		return true;
	case '3':
		SetMode(ToolMode::Annotate);
		return true;
	case WXK_SPACE:
		RunSearch(false);
		return true;
	case 'A':
		ToggleAnimation();
		return true;
	case 'N':
	case WXK_RIGHT:
		StepOnce();
		return true;
	case WXK_LEFT:
		ShowStep(playback.DisplayStep() - stepInterval);
		return true;
	case WXK_PAGEDOWN:
		MoveByQuarters(1);
		return true;
	case WXK_PAGEUP:
		MoveByQuarters(-1);
		return true;
	case WXK_HOME:
		ShowStep(0);
		return true;
	case WXK_END:
		ShowStep(std::numeric_limits<int>::max());
		return true;
	case WXK_ESCAPE:
	{
		// Esc leaves full screen, otherwise clears the path
		if (IsFullScreen())
		{
			SetFullScreen(false);
		}
		else
		{
			ClearSearch();
		}
		return true;
	}
	default:
		return false;
	}
}

#pragma endregion
