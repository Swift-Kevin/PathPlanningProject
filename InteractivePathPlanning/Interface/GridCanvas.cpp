#pragma region Includes

#include "Interface/GridCanvas.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/menu.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#pragma endregion

namespace
{
	// Space around the grid in DIPs
	constexpr int CANVAS_MARGIN = 20;

	// Cost painting tick, about 60 fps
	constexpr int PAINT_FRAME_MS = 16;
	// Seconds of paint applied on the first click
	constexpr float PAINT_DAB_SECONDS = 0.1f;
	// Max seconds of paint in one frame
	constexpr float MAX_PAINT_FRAME_SECONDS = 0.1f;

	// Search tint strength, 0 to 1
	constexpr float VISITED_TINT = 0.6f;
	constexpr float FRONTIER_TINT = 0.7f;

	// Smallest cell size in pixels that shows the Start and End tags
	constexpr int MIN_CELL_PX_FOR_TAGS = 16;
	// Smallest cell size in pixels that shows cost numbers
	constexpr int MIN_CELL_PX_FOR_NUMBERS = 22;

	// Annotation sizes in DIPs
	constexpr int DRAWING_LINE_WIDTH = 3;
	constexpr int DRAWING_MIN_POINT_GAP = 2;
	constexpr int ERASER_RADIUS = 10;

	// Blends two colours, 0 gives _from and 1 gives _to
	wxColour Mix(const wxColour& _from, const wxColour& _to, float _amount)
	{
		const float t = std::clamp(_amount, 0.0f, 1.0f);
		auto channel = [t](unsigned char _a, unsigned char _b)
			{
				return static_cast<unsigned char>(std::lround(_a + (_b - _a) * t));
			};

		return wxColour(channel(_from.Red(), _to.Red()), channel(_from.Green(), _to.Green()), channel(_from.Blue(), _to.Blue()));
	}

	wxColour WithAlpha(const wxColour& _colour, unsigned char _alpha)
	{
		return wxColour(_colour.Red(), _colour.Green(), _colour.Blue(), _alpha);
	}

	// Distance from _point to the segment _a to _b
	double DistanceToSegment(const wxPoint2DDouble& _point, const wxPoint2DDouble& _a, const wxPoint2DDouble& _b)
	{
		const wxPoint2DDouble segment = _b - _a;
		const double lengthSquared = segment.m_x * segment.m_x + segment.m_y * segment.m_y;
		double along = 0.0;

		if (lengthSquared > 0.0)
		{
			const wxPoint2DDouble toPoint = _point - _a;
			along = std::clamp((toPoint.m_x * segment.m_x + toPoint.m_y * segment.m_y) / lengthSquared, 0.0, 1.0);
		}

		const wxPoint2DDouble closest(_a.m_x + segment.m_x * along, _a.m_y + segment.m_y * along);
		return std::hypot(_point.m_x - closest.m_x, _point.m_y - closest.m_y);
	}

	// Gap between cells, only when cells are big enough
	int GutterFor(int _cellPx)
	{
		if (_cellPx >= 22)
		{
			return 2;
		}

		return _cellPx >= 7 ? 1 : 0;
	}

	// Cells on the line from _from to _to (Bresenham), so fast drags don't skip cells
	std::vector<GridPos> CellsOnLine(GridPos _from, GridPos _to)
	{
		std::vector<GridPos> cells;
		int x = _from.x;
		int y = _from.y;
		const int dx = std::abs(_to.x - x);
		const int dy = -std::abs(_to.y - y);
		const int stepX = x < _to.x ? 1 : -1;
		const int stepY = y < _to.y ? 1 : -1;
		int error = dx + dy;

		while (true)
		{
			cells.push_back({ x, y });

			if (x == _to.x && y == _to.y)
			{
				return cells;
			}

			const int doubledError = 2 * error;

			if (doubledError >= dy)
			{
				error += dy;
				x += stepX;
			}

			if (doubledError <= dx)
			{
				error += dx;
				y += stepY;
			}
		}
	}
}

GridCanvas::GridCanvas(wxWindow* _parent, Grid& _grid, const Palette& _palette) : wxWindow(_parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
, grid(_grid), palette(_palette), paintTimer(this)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	Bind(wxEVT_PAINT, &GridCanvas::OnPaint, this);
	Bind(wxEVT_SIZE, &GridCanvas::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &GridCanvas::OnMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &GridCanvas::OnMouseDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GridCanvas::OnMouseDown, this);
	Bind(wxEVT_RIGHT_DCLICK, &GridCanvas::OnMouseDown, this);
	Bind(wxEVT_LEFT_UP, &GridCanvas::OnMouseUp, this);
	Bind(wxEVT_RIGHT_UP, &GridCanvas::OnMouseUp, this);
	Bind(wxEVT_MOTION, &GridCanvas::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &GridCanvas::OnLeave, this);
	Bind(wxEVT_MOUSEWHEEL, &GridCanvas::OnWheel, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &GridCanvas::OnCaptureLost, this);
	Bind(wxEVT_KEY_DOWN, &GridCanvas::OnKeyDown, this);
	Bind(wxEVT_TIMER, &GridCanvas::OnPaintTimer, this, paintTimer.GetId());

	UpdateCursor();
}

#pragma region Public

void GridCanvas::SetMode(ToolMode _mode)
{
	EndDrag();
	mode = _mode;
	UpdateCursor();
	Refresh(false);
}

void GridCanvas::SetBrushRadius(int _radius)
{
	brushRadius = std::clamp(_radius, 0, MAX_BRUSH_RADIUS);
	Refresh(false);
}

void GridCanvas::SetSearch(std::shared_ptr<const SearchResult> _result, int _displayStep)
{
	search = std::move(_result);
	displayStep = _displayStep;
	cellsDirty = true;
	Refresh(false);
}

void GridCanvas::SetVisibleLayers(const VisibleLayers& _layers)
{
	layers = _layers;
	cellsDirty = true;
	Refresh(false);
}

void GridCanvas::ClearDrawings()
{
	drawings.clear();
	Refresh(false);
}

void GridCanvas::RefreshCells()
{
	// the grid may have shrunk under the mouse
	if (hover && !grid.InBounds(*hover))
	{
		hover.reset();
	}

	UpdateLayout(GetClientSize());
	cellsDirty = true;
	Refresh(false);
}

wxBitmap GridCanvas::RenderImage(wxSize _size)
{
	// lay out for the image size, draw, then go back to the window layout
	UpdateLayout(_size);
	cellsDirty = true;
	wxBitmap image(_size, 24);

	{
		wxMemoryDC dc(image);
		DrawScene(dc, false);
	}

	RefreshCells();
	return image;
}

#pragma endregion

#pragma region Layout

void GridCanvas::UpdateLayout(wxSize _area)
{
	const int margin = FromDIP(CANVAS_MARGIN);
	const int availableWidth = std::max(1, _area.x - 2 * margin);
	const int availableHeight = std::max(1, _area.y - 2 * margin);

	// whole pixel cell size so every cell matches, the gutter depends on the size
	const int roughCellPx = std::min(availableWidth / grid.Width(), availableHeight / grid.Height());
	gutter = GutterFor(roughCellPx);
	cellPx = std::max(2, std::min((availableWidth - gutter) / grid.Width(), (availableHeight - gutter) / grid.Height()));
	gutter = GutterFor(cellPx);

	const int gridWidth = grid.Width() * cellPx + gutter;
	const int gridHeight = grid.Height() * cellPx + gutter;
	origin = wxPoint((_area.x - gridWidth) / 2, (_area.y - gridHeight) / 2);
}

std::optional<GridPos> GridCanvas::CellAt(wxPoint _point) const
{
	const int localX = _point.x - origin.x - gutter;
	const int localY = _point.y - origin.y - gutter;

	if (localX < 0 || localY < 0)
	{
		return std::nullopt;
	}

	const GridPos cell{ localX / cellPx, localY / cellPx };

	if (!grid.InBounds(cell))
	{
		return std::nullopt;
	}

	return cell;
}

wxRect GridCanvas::CellRect(GridPos _cell) const
{
	return wxRect(origin.x + gutter + _cell.x * cellPx, origin.y + gutter + _cell.y * cellPx, cellPx - gutter, cellPx - gutter);
}

wxPoint2DDouble GridCanvas::CellCenter(GridPos _cell) const
{
	const wxRect rect = CellRect(_cell);
	return wxPoint2DDouble(rect.x + rect.width / 2.0, rect.y + rect.height / 2.0);
}

wxPoint2DDouble GridCanvas::PixelsToCells(wxPoint _point) const
{
	return wxPoint2DDouble(static_cast<double>(_point.x - origin.x - gutter) / cellPx, static_cast<double>(_point.y - origin.y - gutter) / cellPx);
}

wxPoint2DDouble GridCanvas::CellsToPixels(const wxPoint2DDouble& _point) const
{
	return wxPoint2DDouble(origin.x + gutter + _point.m_x * cellPx, origin.y + gutter + _point.m_y * cellPx);
}

#pragma endregion

#pragma region Drawing

bool GridCanvas::SearchVisible() const
{
	return search && search->closedStep.size() == static_cast<size_t>(grid.CellCount());
}

wxColour GridCanvas::CellColour(int _index) const
{
	const Cell& cell = grid.At(_index);

	// with the map hidden, tiles use the background colour
	const wxColour blankTile = layers.map ? Theme::EMPTY_CELL : Theme::CANVAS_BACKGROUND;

	if (!cell.walkable)
	{
		return layers.walls ? palette.wall : blankTile;
	}

	wxColour terrain = blankTile;

	if (layers.costs)
	{
		const float costAmount = (cell.cost - Grid::MIN_COST) / (Grid::MAX_COST - Grid::MIN_COST);
		terrain = Mix(blankTile, palette.highCost, costAmount);
	}

	if (!SearchVisible())
	{
		return terrain;
	}

	// visited cells were on the frontier first, so check visited first
	const bool isVisited = search->closedStep[_index] <= displayStep;
	const bool isFrontier = !isVisited && search->openedStep[_index] <= displayStep;

	// tint so the cost colour still shows
	if (isVisited && layers.visited)
	{
		return Mix(terrain, palette.visited, VISITED_TINT);
	}

	if (isFrontier && layers.frontier)
	{
		return Mix(terrain, palette.frontier, FRONTIER_TINT);
	}

	return terrain;
}

void GridCanvas::RebuildCellsBitmap()
{
	const int width = grid.Width();
	const int height = grid.Height();
	const int imageWidth = width * cellPx + gutter;
	const int imageHeight = height * cellPx + gutter;
	const int cellInnerPx = cellPx - gutter;
	const size_t rowBytes = static_cast<size_t>(imageWidth) * 3;

	wxImage image(imageWidth, imageHeight, false);
	unsigned char* pixels = image.GetData();

	// one pixel row of gutter colour for the gaps between rows
	const wxColour gutterColour = layers.map ? Theme::GUTTER : Theme::CANVAS_BACKGROUND;
	std::vector<unsigned char> gutterRow(rowBytes);

	for (int x = 0; x < imageWidth; ++x)
	{
		gutterRow[x * 3 + 0] = gutterColour.Red();
		gutterRow[x * 3 + 1] = gutterColour.Green();
		gutterRow[x * 3 + 2] = gutterColour.Blue();
	}

	std::vector<unsigned char> cellRow(gutterRow);

	for (int y = 0; y < height; ++y)
	{
		// build one pixel row for this row of cells
		for (int x = 0; x < width; ++x)
		{
			const wxColour colour = CellColour(grid.Index({ x, y }));
			unsigned char* out = cellRow.data() + static_cast<size_t>(gutter + x * cellPx) * 3;

			for (int px = 0; px < cellInnerPx; ++px)
			{
				out[px * 3 + 0] = colour.Red();
				out[px * 3 + 1] = colour.Green();
				out[px * 3 + 2] = colour.Blue();
			}
		}

		// then copy it down for the cell height, after the gutter rows
		const int rowTop = y * cellPx;

		for (int py = 0; py < gutter; ++py)
		{
			std::memcpy(pixels + (rowTop + py) * rowBytes, gutterRow.data(), rowBytes);
		}

		for (int py = gutter; py < cellPx; ++py)
		{
			std::memcpy(pixels + (rowTop + py) * rowBytes, cellRow.data(), rowBytes);
		}
	}

	// bottom edge gutter
	for (int py = height * cellPx; py < imageHeight; ++py)
	{
		std::memcpy(pixels + py * rowBytes, gutterRow.data(), rowBytes);
	}

	cellsBitmap = wxBitmap(image);
	cellsDirty = false;
}

void GridCanvas::DrawCurrentExpansion(wxGraphicsContext& _gc) const
{
	const bool isPlayingBack = SearchVisible() && displayStep > 0 && displayStep < search->TotalSteps();

	// hidden with the visited layer
	if (!isPlayingBack || !layers.visited)
	{
		return;
	}

	const wxRect rect = CellRect(grid.PosOf(search->expansionOrder[displayStep - 1]));
	const double width = std::max(1.5, (cellPx - gutter) * 0.12);
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(palette.visited.ChangeLightness(55), width)));
	_gc.SetBrush(*wxTRANSPARENT_BRUSH);
	_gc.DrawRectangle(rect.x + width / 2, rect.y + width / 2, rect.width - width, rect.height - width);
}

void GridCanvas::DrawPath(wxGraphicsContext& _gc) const
{
	const bool isFinished = SearchVisible() && search->found && displayStep >= search->TotalSteps();

	if (!layers.path || !isFinished || search->path.size() < 2)
	{
		return;
	}

	std::vector<wxPoint2DDouble> points;
	points.reserve(search->path.size());

	for (const GridPos& cell : search->path)
	{
		points.push_back(CellCenter(cell));
	}

	// light halo under the line so it shows on any colour
	const double width = std::max<double>(FromDIP(2), (cellPx - gutter) * 0.3);
	const double haloWidth = width + std::max(2.0, width * 0.5);
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(WithAlpha(Theme::EMPTY_CELL, 170), haloWidth).Cap(wxCAP_ROUND).Join(wxJOIN_ROUND)));
	_gc.StrokeLines(points.size(), points.data());
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(palette.path, width).Cap(wxCAP_ROUND).Join(wxJOIN_ROUND)));
	_gc.StrokeLines(points.size(), points.data());
}

void GridCanvas::DrawHoverOutline(wxGraphicsContext& _gc) const
{
	if (!hover || mode != ToolMode::TileState || drag != DragAction::None)
	{
		return;
	}

	const wxRect rect = CellRect(*hover);
	const double width = FromDIP(2);
	const double cornerRadius = std::min(4.0, (cellPx - gutter) / 4.0);

	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(WithAlpha(palette.highlight, 130), width)));
	_gc.SetBrush(*wxTRANSPARENT_BRUSH);
	// outline is centred on the cell edge
	_gc.DrawRoundedRectangle(rect.x - width / 2 + 0.5, rect.y - width / 2 + 0.5,
		rect.width + width - 1, rect.height + width - 1, cornerRadius);
}

void GridCanvas::DrawEndpoint(wxGraphicsContext& _gc, GridPos _cell, const wxColour& _colour) const
{
	const wxPoint2DDouble center = CellCenter(_cell);
	const double radius = std::max(3.0, (cellPx - gutter) * 0.42);

	// white ring around the marker when there's room
	const double ringWidth = radius >= 6 ? std::max(1.5, radius * 0.16) : 1.0;
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(Theme::EMPTY_CELL, ringWidth)));
	_gc.SetBrush(wxBrush(_colour));
	_gc.DrawEllipse(center.m_x - radius, center.m_y - radius, radius * 2, radius * 2);
}

void GridCanvas::DrawEndpointTag(wxGraphicsContext& _gc, GridPos _cell, const wxColour& _colour, const wxString& _name) const
{
	const int cellInnerPx = cellPx - gutter;

	if (cellInnerPx < MIN_CELL_PX_FOR_TAGS)
	{
		return;
	}

	wxFont font = GetFont().Bold();
	font.SetPixelSize(wxSize(0, std::max(FromDIP(10), static_cast<int>(cellInnerPx * 0.34))));
	_gc.SetFont(font, *wxWHITE);

	double textWidth = 0;
	double textHeight = 0;
	_gc.GetTextExtent(_name, &textWidth, &textHeight);

	const double paddingX = textHeight * 0.45;
	const double paddingY = textHeight * 0.12;
	const double tagWidth = textWidth + paddingX * 2;
	const double tagHeight = textHeight + paddingY * 2;
	const double markerRadius = cellInnerPx * 0.42;
	const wxPoint2DDouble center = CellCenter(_cell);

	// above the marker, or below it on the top row
	const double gap = FromDIP(3);
	const double tagTop = _cell.y == 0 ? center.m_y + markerRadius + gap : center.m_y - markerRadius - tagHeight - gap;

	const double tagLeft = center.m_x - tagWidth / 2;
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(Theme::EMPTY_CELL, FromDIP(1.5))));
	_gc.SetBrush(wxBrush(_colour));
	_gc.DrawRoundedRectangle(tagLeft, tagTop, tagWidth, tagHeight, tagHeight / 2);
	_gc.DrawText(_name, tagLeft + paddingX, tagTop + paddingY);
}

void GridCanvas::DrawBrush(wxGraphicsContext& _gc) const
{
	if (mode != ToolMode::CostPaint || !hover)
	{
		return;
	}

	const wxPoint2DDouble center = CellCenter(*hover);
	const double radius = (brushRadius + 0.5) * cellPx;
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(WithAlpha(palette.highlight, 220), FromDIP(2))));
	_gc.SetBrush(wxBrush(WithAlpha(palette.highlight, 34)));
	_gc.DrawEllipse(center.m_x - radius, center.m_y - radius, radius * 2, radius * 2);
}

void GridCanvas::DrawDrawings(wxGraphicsContext& _gc) const
{
	if (!layers.drawings || drawings.empty())
	{
		return;
	}

	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(palette.drawing, FromDIP(DRAWING_LINE_WIDTH)).Cap(wxCAP_ROUND).Join(wxJOIN_ROUND)));

	for (const std::vector<wxPoint2DDouble>& drawing : drawings)
	{
		std::vector<wxPoint2DDouble> points;
		points.reserve(drawing.size() + 1);

		for (const wxPoint2DDouble& point : drawing)
		{
			points.push_back(CellsToPixels(point));
		}

		// a click with no drag still draws a dot
		if (points.size() == 1)
		{
			points.push_back(points.front() + wxPoint2DDouble(0.1, 0.0));
		}

		_gc.StrokeLines(points.size(), points.data());
	}
}

void GridCanvas::DrawCostNumbers(wxGraphicsContext& _gc) const
{
	const int cellInnerPx = cellPx - gutter;

	if (!layers.costNumbers || cellInnerPx < MIN_CELL_PX_FOR_NUMBERS)
	{
		return;
	}

	// slightly see-through dark text
	wxFont font = GetFont().Bold();
	font.SetPixelSize(wxSize(0, static_cast<int>(cellInnerPx * 0.4)));
	_gc.SetFont(font, WithAlpha(Theme::TEXT, 150));

	for (int index = 0; index < grid.CellCount(); ++index)
	{
		const Cell& cell = grid.At(index);

		if (!cell.walkable || cell.cost <= Grid::MIN_COST)
		{
			continue;
		}

		// whole costs show as 5, painted ones as 3.7
		const bool isWhole = cell.cost == std::round(cell.cost);
		const wxString text = wxString::Format(isWhole ? "%.0f" : "%.1f", cell.cost);

		double textWidth = 0;
		double textHeight = 0;
		_gc.GetTextExtent(text, &textWidth, &textHeight);

		const wxPoint2DDouble center = CellCenter(grid.PosOf(index));
		_gc.DrawText(text, center.m_x - textWidth / 2, center.m_y - textHeight / 2);
	}
}

void GridCanvas::DrawEraser(wxGraphicsContext& _gc) const
{
	if (drag != DragAction::Erase)
	{
		return;
	}

	const double radius = FromDIP(ERASER_RADIUS);
	_gc.SetPen(_gc.CreatePen(wxGraphicsPenInfo(WithAlpha(palette.drawing, 220), FromDIP(2))));
	_gc.SetBrush(wxBrush(WithAlpha(palette.drawing, 40)));
	_gc.DrawEllipse(mousePosition.x - radius, mousePosition.y - radius, radius * 2, radius * 2);
}

#pragma endregion

#pragma region Editing

void GridCanvas::BeginDrag(GridPos _cell, bool _rightButton)
{
	if (mode == ToolMode::TileState)
	{
		if (_rightButton)
		{
			ShowEndpointMenu(_cell);
			return;
		}

		// drag the start or end marker to move it
		if (grid.IsEndpoint(_cell))
		{
			StartDrag(_cell == grid.Start() ? DragAction::MoveStart : DragAction::MoveEnd, _cell);
			return;
		}

		// the whole stroke paints what the first cell became
		StartDrag(DragAction::PaintWalls, _cell);
		wallStrokeMakesWalkable = !grid.At(_cell).walkable;
		RecordStrokeChange(grid.SetWalkable(_cell, wallStrokeMakesWalkable));
		return;
	}

	// cost painter, left raises and right lowers while held
	StartDrag(DragAction::PaintCost, _cell);
	paintRaisesCost = !_rightButton;
	lastPaintedCell = _cell;
	lastPaintTime = Clock::now();
	RecordStrokeChange(PaintCostAt(_cell, paintRate * PAINT_DAB_SECONDS));
	paintTimer.Start(PAINT_FRAME_MS);
}

void GridCanvas::StartDrag(DragAction _action, GridPos _cell)
{
	NotifyEditStarting();
	drag = _action;
	lastDragCell = _cell;
	strokeChangedGrid = false;

	if (!HasCapture())
	{
		CaptureMouse();
	}

	UpdateCursor();
}

void GridCanvas::ContinueDrag(GridPos _cell)
{
	if (_cell == lastDragCell)
	{
		return;
	}

	switch (drag)
	{
	case DragAction::MoveStart:
		// endpoints search again right away so the path follows
		if (grid.SetStart(_cell))
		{
			NotifyEdited(EditPhase::Finished);
		}
		break;
	case DragAction::MoveEnd:
		if (grid.SetEnd(_cell))
		{
			NotifyEdited(EditPhase::Finished);
		}
		break;
	case DragAction::PaintWalls:
	{
		// first cell was painted last event
		const std::vector<GridPos> line = CellsOnLine(lastDragCell, _cell);
		bool changed = false;

		for (size_t i = 1; i < line.size(); ++i)
		{
			changed |= grid.SetWalkable(line[i], wallStrokeMakesWalkable);
		}

		RecordStrokeChange(changed);
		break;
	}
	case DragAction::PaintCost:
		// the paint timer does the painting
		break;
	case DragAction::None:
		return;
	}

	lastDragCell = _cell;
}

void GridCanvas::EndDrag()
{
	paintTimer.Stop();

	if (HasCapture())
	{
		ReleaseMouse();
	}

	if (drag == DragAction::None)
	{
		return;
	}

	const bool finishedAStroke = strokeChangedGrid && (drag == DragAction::PaintWalls || drag == DragAction::PaintCost);
	drag = DragAction::None;
	strokeChangedGrid = false;
	UpdateCursor();
	Refresh(false);

	// search again once the stroke ends
	if (finishedAStroke)
	{
		NotifyEdited(EditPhase::Finished);
	}
}

void GridCanvas::RecordStrokeChange(bool _changed)
{
	if (_changed)
	{
		strokeChangedGrid = true;
		NotifyEdited(EditPhase::Ongoing);
	}
}

bool GridCanvas::PaintCostAt(GridPos _center, float _amount)
{
	bool changed = false;
	const float brushReach = brushRadius + 0.5f;
	const float direction = paintRaisesCost ? 1.0f : -1.0f;

	for (int dy = -brushRadius; dy <= brushRadius; ++dy)
	{
		for (int dx = -brushRadius; dx <= brushRadius; ++dx)
		{
			const GridPos cell{ _center.x + dx, _center.y + dy };
			const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

			if (!grid.InBounds(cell) || distance > brushReach)
			{
				continue;
			}

			// full strength in the middle, fading out to the edge
			const float falloff = 1.0f - distance / (brushRadius + 1.0f);
			changed |= grid.AddCost(cell, direction * _amount * falloff);
		}
	}

	return changed;
}

void GridCanvas::ChangeBrushRadius(int _change)
{
	SetBrushRadius(brushRadius + _change);

	if (onBrushRadiusChanged)
	{
		onBrushRadiusChanged(brushRadius);
	}
}

void GridCanvas::NotifyEditStarting()
{
	if (onEditStarting)
	{
		onEditStarting();
	}
}

void GridCanvas::NotifyEdited(EditPhase _phase)
{
	cellsDirty = true;
	Refresh(false);

	if (onGridEdited)
	{
		onGridEdited(_phase);
	}
}

void GridCanvas::ShowEndpointMenu(GridPos _cell)
{
	enum
	{
		ID_SET_START = wxID_HIGHEST + 1,
		ID_SET_END,
	};

	const bool canBeEndpoint = grid.At(_cell).walkable && !grid.IsEndpoint(_cell);

	wxMenu menu;
	menu.Append(ID_SET_START, "Set as Start")->Enable(canBeEndpoint);
	menu.Append(ID_SET_END, "Set as End")->Enable(canBeEndpoint);

	const int choice = GetPopupMenuSelectionFromUser(menu);

	if (choice != ID_SET_START && choice != ID_SET_END)
	{
		return;
	}

	NotifyEditStarting();
	const bool changed = choice == ID_SET_START ? grid.SetStart(_cell) : grid.SetEnd(_cell);

	if (changed)
	{
		NotifyEdited(EditPhase::Finished);
	}
}

#pragma endregion

#pragma region Annotating

void GridCanvas::BeginAnnotation(wxPoint _point, bool _rightButton)
{
	drag = _rightButton ? DragAction::Erase : DragAction::Draw;
	mousePosition = _point;

	if (!HasCapture())
	{
		CaptureMouse();
	}

	if (drag == DragAction::Draw)
	{
		drawings.push_back({ PixelsToCells(_point) });
	}
	else
	{
		EraseDrawingsNear(_point);
	}

	UpdateCursor();
	Refresh(false);
}

void GridCanvas::ContinueAnnotation(wxPoint _point)
{
	mousePosition = _point;

	if (drag == DragAction::Erase)
	{
		EraseDrawingsNear(_point);
	}
	else if (!drawings.empty())
	{
		// skip points too close to the last one
		std::vector<wxPoint2DDouble>& drawing = drawings.back();
		const wxPoint2DDouble lastPoint = CellsToPixels(drawing.back());

		if (std::hypot(_point.x - lastPoint.m_x, _point.y - lastPoint.m_y) >= FromDIP(DRAWING_MIN_POINT_GAP))
		{
			drawing.push_back(PixelsToCells(_point));
		}
	}

	Refresh(false);
}

void GridCanvas::EraseDrawingsNear(wxPoint _point)
{
	const wxPoint2DDouble eraser(_point.x, _point.y);
	const double reach = FromDIP(ERASER_RADIUS);

	// erase a drawing if any part is in reach
	std::erase_if(drawings, [&](const std::vector<wxPoint2DDouble>& _drawing)
		{
			for (size_t i = 0; i < _drawing.size(); ++i)
			{
				const wxPoint2DDouble from = CellsToPixels(_drawing[i]);
				const wxPoint2DDouble to = CellsToPixels(_drawing[std::min(i + 1, _drawing.size() - 1)]);

				if (DistanceToSegment(eraser, from, to) <= reach)
				{
					return true;
				}
			}

			return false;
		});
}

#pragma endregion

#pragma region Hover

void GridCanvas::SetHover(std::optional<GridPos> _cell)
{
	if (_cell == hover)
	{
		return;
	}

	hover = _cell;
	UpdateCursor();
	Refresh(false);
}

void GridCanvas::UpdateCursor()
{
	const bool overEndpoint = hover && grid.IsEndpoint(*hover);

	if (drag == DragAction::MoveStart || drag == DragAction::MoveEnd)
	{
		SetCursor(wxCursor(wxCURSOR_SIZING));
	}
	else if (mode == ToolMode::CostPaint)
	{
		SetCursor(wxCursor(wxCURSOR_CROSS));
	}
	else if (mode == ToolMode::Annotate)
	{
		SetCursor(wxCursor(drag == DragAction::Erase ? wxCURSOR_BLANK : wxCURSOR_PENCIL));
	}
	else if (overEndpoint)
	{
		SetCursor(wxCursor(wxCURSOR_HAND));
	}
	else
	{
		SetCursor(wxCursor(wxCURSOR_PENCIL));
	}
}

#pragma endregion

#pragma region Scene

void GridCanvas::DrawScene(wxDC& _dc, bool _showMouse)
{
	_dc.SetBackground(wxBrush(Theme::CANVAS_BACKGROUND));
	_dc.Clear();

	if (cellsDirty || !cellsBitmap.IsOk())
	{
		RebuildCellsBitmap();
	}

	_dc.DrawBitmap(cellsBitmap, origin);

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(_dc));

	if (!gc)
	{
		return;
	}

	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

	// drawn back to front
	DrawCostNumbers(*gc);
	DrawCurrentExpansion(*gc);
	DrawPath(*gc);

	if (_showMouse)
	{
		DrawHoverOutline(*gc);
	}

	DrawEndpoint(*gc, grid.Start(), palette.start);
	DrawEndpoint(*gc, grid.End(), palette.end);
	DrawEndpointTag(*gc, grid.Start(), palette.start, "Start");
	DrawEndpointTag(*gc, grid.End(), palette.end, "End");
	DrawDrawings(*gc);

	if (_showMouse)
	{
		DrawBrush(*gc);
		DrawEraser(*gc);
	}
}

#pragma endregion

#pragma region Events

void GridCanvas::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	DrawScene(dc, true);
}

void GridCanvas::OnSize(wxSizeEvent& _event)
{
	RefreshCells();
	_event.Skip();
}

void GridCanvas::OnMouseDown(wxMouseEvent& _event)
{
	SetFocus();
	const bool rightButton = _event.GetButton() == wxMOUSE_BTN_RIGHT;

	// annotations can go anywhere, not just on cells
	if (mode == ToolMode::Annotate)
	{
		BeginAnnotation(_event.GetPosition(), rightButton);
		return;
	}

	const std::optional<GridPos> cell = CellAt(_event.GetPosition());

	if (cell)
	{
		BeginDrag(*cell, rightButton);
	}
}

void GridCanvas::OnMouseUp(wxMouseEvent&)
{
	EndDrag();
}

void GridCanvas::OnMotion(wxMouseEvent& _event)
{
	const std::optional<GridPos> cell = CellAt(_event.GetPosition());
	SetHover(cell);

	if (drag == DragAction::None)
	{
		return;
	}

	// end the drag if the button up was missed
	if (!_event.LeftIsDown() && !_event.RightIsDown())
	{
		EndDrag();
		return;
	}

	if (drag == DragAction::Draw || drag == DragAction::Erase)
	{
		ContinueAnnotation(_event.GetPosition());
		return;
	}

	if (cell)
	{
		ContinueDrag(*cell);
	}
}

void GridCanvas::OnLeave(wxMouseEvent&)
{
	if (drag == DragAction::None)
	{
		SetHover(std::nullopt);
	}
}

void GridCanvas::OnWheel(wxMouseEvent& _event)
{
	// wheel resizes the cost brush
	if (mode != ToolMode::CostPaint || _event.GetWheelRotation() == 0)
	{
		_event.Skip();
		return;
	}

	ChangeBrushRadius(_event.GetWheelRotation() > 0 ? 1 : -1);
}

void GridCanvas::OnCaptureLost(wxMouseCaptureLostEvent&)
{
	EndDrag();
}

void GridCanvas::OnKeyDown(wxKeyEvent& _event)
{
	if (onKey && onKey(_event))
	{
		return;
	}

	// [ and ] resize the brush
	const wxChar character = _event.GetUnicodeKey();

	if ((character == '[' || character == ']') && mode == ToolMode::CostPaint)
	{
		ChangeBrushRadius(character == ']' ? 1 : -1);
		return;
	}

	_event.Skip();
}

void GridCanvas::OnPaintTimer(wxTimerEvent&)
{
	if (drag != DragAction::PaintCost)
	{
		paintTimer.Stop();
		return;
	}

	// delta time calc
	const Clock::time_point now = Clock::now();
	const float seconds = std::min(std::chrono::duration<float>(now - lastPaintTime).count(), MAX_PAINT_FRAME_SECONDS);
	lastPaintTime = now;

	// spread the paint along the brush's path
	const std::vector<GridPos> stroke = CellsOnLine(lastPaintedCell, lastDragCell);
	const float amountPerCell = paintRate * seconds / static_cast<float>(stroke.size());
	bool changed = false;

	for (const GridPos& cell : stroke)
	{
		changed |= PaintCostAt(cell, amountPerCell);
	}

	lastPaintedCell = lastDragCell;
	RecordStrokeChange(changed);
}

#pragma endregion
