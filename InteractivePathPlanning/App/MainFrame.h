#pragma once

#include "Interface/GridCanvas.h"
#include "Interface/Style/Theme.h"
#include "Map/EditHistory.h"
#include "Map/Grid.h"
#include "Search/PathFinder.h"
#include "Search/SearchPlayback.h"

#include <wx/frame.h>
#include <wx/timer.h>

#include <chrono>
#include <cstdint>
#include <functional>

class CollapseHandle;
class FlatToolBar;
class SidePanel;
class wxPanel;
class wxStaticText;

// Main window, connects the toolbar, canvas and side panel to the grid and search
class MainFrame : public wxFrame
{
private:
	using Clock = std::chrono::steady_clock;

	// Map data
	Grid grid;
	Palette palette;
	// Layers picked in the View menu
	VisibleLayers layers;
	// Undo and redo
	EditHistory history;

	// Searching
	SearchSettings settings;
	SearchPlayback playback;
	// Set by edits, the search runs again on idle
	bool searchRerunPending = false;
	// Steps per Step button press
	int stepInterval = 1;

	// Side panel open state, hidden in full screen either way
	bool sidePanelOpen = true;

	// Animation
	wxTimer animationTimer;
	Clock::time_point lastAnimationTick;

	// Widgets
	FlatToolBar* toolBar = nullptr;
	GridCanvas* canvas = nullptr;
	CollapseHandle* sideHandle = nullptr;
	SidePanel* side = nullptr;
	// Status strip along the bottom
	wxPanel* statusDivider = nullptr;
	wxPanel* statusStrip = nullptr;
	wxStaticText* statusText = nullptr;

#pragma region Setup

	// Creates the toolbar, canvas, side panel and status strip
	void BuildLayout();
	void BuildToolBar();
	void BuildStatusStrip();

	// Binds control events
	void BindEvents();
	void BindCanvas();
	void BindSearchControls();
	void BindToolControls();
	void BindGridControls();
	void BindColourControls();
	void BindKeyboardShortcuts();

	// Resets the map, settings, colours and tool
	void LoadFactoryState();
	// Asks before resetting everything
	void ConfirmFactoryReset();
	// Loads a built-in map
	void LoadMap(int _index);
	// Makes a random map from _seed at the Map card size
	void GenerateMap(std::uint32_t _seed);

#pragma endregion

#pragma region Toolbar Menus

	// View menu layer checklist
	void ShowViewMenu();
	// Clear menu for walls, costs, path and drawings
	void ShowClearMenu();
	// Step interval menu
	void ShowStepMenu();
	// Opens the Help window
	void ShowHelp();
	// Saves the grid as a 1920 x 1080 PNG
	void SaveImage();

#pragma endregion

#pragma region Searching

	// Reads the search options from the side panel
	void ReadSettings();
	// Runs a search, shown finished or animated
	void RunSearch(bool _animate);
	// Animate, Pause and Resume button
	void ToggleAnimation();
	// Shows the next stepInterval steps
	void StepOnce();
	// Jumps to _step
	void ShowStep(int _step);
	// Jumps forward (1) or back (-1) a quarter of the search
	void MoveByQuarters(int _quarters);
	// Clears the search
	void ClearSearch();
	// Starts the animation timer
	void StartAnimationTimer();
	// Sends the search to the canvas and updates the toolbar and status
	void ShowSearch();

#pragma endregion

#pragma region Reacting To Changes

	// Called when the canvas edits the grid
	void MapEdited(EditPhase _phase);
	// Called after the grid changes outside the canvas
	void GridChangedOutsideCanvas();
	// Saves for undo, runs _change, then refreshes
	void ChangeGrid(const std::function<void()>& _change);
	// Edit history
	void Undo();
	void Redo();
	// Called when a search option changes
	void SettingsChanged();

#pragma endregion

#pragma region UI Sync

	void SetMode(ToolMode _mode);
	// Opens or closes the side panel
	void SetSidePanelOpen(bool _open);
	// Full screen shows only the grid
	void SetFullScreen(bool _fullScreen);
	void UpdateToolBar();
	void UpdateStepLabel();
	void UpdateStats();
	void SyncSizeInputs();
	void SyncColourPickers();
	void ReadColourPickers();

#pragma endregion

	// Events
	void OnToolBarButton(wxCommandEvent& _event);
	void OnMapChosen(wxCommandEvent& _event);
	void OnAnimationTick(wxTimerEvent& _event);
	void OnIdle(wxIdleEvent& _event);
	bool OnCanvasKey(const wxKeyEvent& _event);

public:
	MainFrame();
};
