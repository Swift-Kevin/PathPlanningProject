#pragma once

#include "Interface/Style/Theme.h"
#include "Map/Grid.h"
#include "Search/PathFinder.h"

#include <wx/bitmap.h>
#include <wx/geometry.h>
#include <wx/timer.h>
#include <wx/window.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class wxGraphicsContext;

// Toolbar tools
enum class ToolMode
{
	TileState,
	CostPaint,
	// Draws over the grid without changing the map
	Annotate,
};

// Whether a stroke is still going or finished
enum class EditPhase
{
	Ongoing,
	Finished,
};

// Layers that can be hidden from the View menu
struct VisibleLayers
{
	// Tiles and grid lines
	bool map = true;
	// Shading on cells that cost more than 1
	bool costs = true;
	// Cost written on cells that cost more than 1, when cells are big enough
	bool costNumbers = false;
	// Visited cells
	bool visited = true;
	// Cells in the open list
	bool frontier = true;
	bool walls = true;
	bool path = true;
	// Annotate tool lines
	bool drawings = true;
};

// Draws the grid and search, and handles mouse editing
class GridCanvas : public wxWindow
{
private:
	using Clock = std::chrono::steady_clock;

	// What the held mouse button is doing
	enum class DragAction
	{
		None,
		MoveStart,
		MoveEnd,
		PaintWalls,
		PaintCost,
		// Annotate tool, left draws and right erases
		Draw,
		Erase,
	};

	// What's shown
	Grid& grid;
	const Palette& palette;
	VisibleLayers layers;
	std::shared_ptr<const SearchResult> search;
	int displayStep = 0;

	// Tool State
	ToolMode mode = ToolMode::TileState;
	std::optional<GridPos> hover;

	// Dragging
	DragAction drag = DragAction::None;
	GridPos lastDragCell;
	bool strokeChangedGrid = false;
	bool wallStrokeMakesWalkable = false;

	// Cost painting, repeats every frame while held
	int brushRadius = 2;
	float paintRate = 4.0f;
	bool paintRaisesCost = true;
	wxTimer paintTimer;
	Clock::time_point lastPaintTime;
	GridPos lastPaintedCell;

	// Annotations, points are stored in cell units from the grid's top left
	// so drawings stay on the same cells when the window resizes
	std::vector<std::vector<wxPoint2DDouble>> drawings;
	// Mouse position for the eraser
	wxPoint mousePosition;

	// Layout in pixels, cell (x, y) starts at origin + gutter + (x, y) * cellPx
	int cellPx = 1;
	int gutter = 0;
	wxPoint origin;

	// Cached cell colours, rebuilt when something changes
	wxBitmap cellsBitmap;
	bool cellsDirty = true;

#pragma region Layout

	// Picks the cell size that fits _area and centres the grid
	void UpdateLayout(wxSize _area);
	// Cell under a window position, if any
	std::optional<GridPos> CellAt(wxPoint _point) const;
	// Cell rectangle in pixels, without the gutter
	wxRect CellRect(GridPos _cell) const;
	wxPoint2DDouble CellCenter(GridPos _cell) const;
	// Pixel and cell conversions for annotations
	wxPoint2DDouble PixelsToCells(wxPoint _point) const;
	wxPoint2DDouble CellsToPixels(const wxPoint2DDouble& _point) const;

#pragma endregion

#pragma region Drawing

	// True if the search matches the grid size
	bool SearchVisible() const;
	// Cell fill from its cost and search state
	wxColour CellColour(int _index) const;
	// Paints every cell into cellsBitmap
	void RebuildCellsBitmap();

	// Overlays drawn on top of the cells
	void DrawCurrentExpansion(wxGraphicsContext& _gc) const;
	void DrawPath(wxGraphicsContext& _gc) const;
	void DrawHoverOutline(wxGraphicsContext& _gc) const;
	void DrawEndpoint(wxGraphicsContext& _gc, GridPos _cell, const wxColour& _colour) const;
	void DrawEndpointTag(wxGraphicsContext& _gc, GridPos _cell, const wxColour& _colour, const wxString& _name) const;
	void DrawBrush(wxGraphicsContext& _gc) const;
	void DrawDrawings(wxGraphicsContext& _gc) const;
	void DrawEraser(wxGraphicsContext& _gc) const;
	void DrawCostNumbers(wxGraphicsContext& _gc) const;

	// Draws everything, _showMouse adds the hover outline, brush and eraser
	void DrawScene(wxDC& _dc, bool _showMouse);

#pragma endregion

#pragma region Editing

	// Starts the current tool's action on a mouse press
	void BeginDrag(GridPos _cell, bool _rightButton);
	// Starts a drag and captures the mouse
	void StartDrag(DragAction _action, GridPos _cell);
	// Updates the drag while a button is held
	void ContinueDrag(GridPos _cell);
	// Ends the drag and reports the change if there was one
	void EndDrag();
	// Reports a stroke change as ongoing
	void RecordStrokeChange(bool _changed);
	// Raises or lowers cost under the brush
	bool PaintCostAt(GridPos _center, float _amount);
	// Resizes the brush and tells the owner
	void ChangeBrushRadius(int _change);
	// Lets the owner save the grid for undo
	void NotifyEditStarting();
	// Tells the owner the grid changed and redraws
	void NotifyEdited(EditPhase _phase);
	// Right click menu to set the start or end
	void ShowEndpointMenu(GridPos _cell);

#pragma endregion

#pragma region Annotating

	// Left starts a drawing, right starts erasing
	void BeginAnnotation(wxPoint _point, bool _rightButton);
	// Adds to the drawing or keeps erasing
	void ContinueAnnotation(wxPoint _point);
	// Removes drawings near _point
	void EraseDrawingsNear(wxPoint _point);

#pragma endregion

	// Updates the hovered cell
	void SetHover(std::optional<GridPos> _cell);
	// Sets the cursor for the tool
	void UpdateCursor();

	// Events
	void OnPaint(wxPaintEvent& _event);
	void OnSize(wxSizeEvent& _event);
	void OnMouseDown(wxMouseEvent& _event);
	void OnMouseUp(wxMouseEvent& _event);
	void OnMotion(wxMouseEvent& _event);
	void OnLeave(wxMouseEvent& _event);
	void OnWheel(wxMouseEvent& _event);
	void OnCaptureLost(wxMouseCaptureLostEvent& _event);
	void OnKeyDown(wxKeyEvent& _event);
	void OnPaintTimer(wxTimerEvent& _event);

public:
	// Largest brush radius in cells
	static constexpr int MAX_BRUSH_RADIUS = 12;

	GridCanvas(wxWindow* _parent, Grid& _grid, const Palette& _palette);

	// Tools
	void SetMode(ToolMode _mode);
	ToolMode Mode() const { return mode; }
	void SetBrushRadius(int _radius);
	int BrushRadius() const { return brushRadius; }
	// Cost per second the brush adds or removes
	void SetPaintRate(float _costPerSecond) { paintRate = _costPerSecond; }

	// Shows a search at _displayStep, nullptr hides it
	void SetSearch(std::shared_ptr<const SearchResult> _result, int _displayStep);
	// Sets which layers are drawn
	void SetVisibleLayers(const VisibleLayers& _layers);

	// Annotations
	bool HasDrawings() const { return !drawings.empty(); }
	void ClearDrawings();

	// Call when the grid or palette changes outside the canvas
	void RefreshCells();

	// Draws the grid into an image of _size, without the mouse overlays
	wxBitmap RenderImage(wxSize _size);

	// Callbacks for the owner
	std::function<void()> onEditStarting;
	std::function<void(EditPhase)> onGridEdited;
	std::function<void(int)> onBrushRadiusChanged;
	// Gets key presses first, return true to use the key
	std::function<bool(const wxKeyEvent&)> onKey;
};
