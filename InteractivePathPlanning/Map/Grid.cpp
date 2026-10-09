#pragma region Includes

#include "Map/Grid.h"

#include <algorithm>

#pragma endregion

Grid::Grid(int _width, int _height)
{
	Reset(_width, _height);
}

void Grid::PlaceDefaultEndpoints()
{
	start = { 1, height / 2 };
	end = { width - 2, height / 2 };
	cells[Index(start)].walkable = true;
	cells[Index(end)].walkable = true;
}

bool Grid::HasWalls() const
{
	for (const Cell& cell : cells)
	{
		if (!cell.walkable)
		{
			return true;
		}
	}

	return false;
}

bool Grid::HasRaisedCosts() const
{
	for (const Cell& cell : cells)
	{
		if (cell.cost > MIN_COST)
		{
			return true;
		}
	}

	return false;
}

bool Grid::SetWalkable(GridPos _pos, bool _walkable)
{
	// the start and end always stay walkable
	if (!InBounds(_pos) || IsEndpoint(_pos) || cells[Index(_pos)].walkable == _walkable)
	{
		return false;
	}

	cells[Index(_pos)].walkable = _walkable;
	++revision;
	return true;
}

bool Grid::SetCost(GridPos _pos, float _cost)
{
	if (!InBounds(_pos))
	{
		return false;
	}

	const float clampedCost = std::clamp(_cost, MIN_COST, MAX_COST);

	if (cells[Index(_pos)].cost == clampedCost)
	{
		return false;
	}

	cells[Index(_pos)].cost = clampedCost;
	++revision;
	return true;
}

bool Grid::SetStart(GridPos _pos)
{
	if (!InBounds(_pos) || !At(_pos).walkable || _pos == start || _pos == end)
	{
		return false;
	}

	start = _pos;
	++revision;
	return true;
}

bool Grid::SetEnd(GridPos _pos)
{
	if (!InBounds(_pos) || !At(_pos).walkable || _pos == start || _pos == end)
	{
		return false;
	}

	end = _pos;
	++revision;
	return true;
}

bool Grid::SetEndpoints(GridPos _start, GridPos _end)
{
	if (!InBounds(_start) || !InBounds(_end) || _start == _end)
	{
		return false;
	}

	start = _start;
	end = _end;
	cells[Index(start)].walkable = true;
	cells[Index(end)].walkable = true;
	++revision;
	return true;
}

void Grid::Reset(int _width, int _height)
{
	width = std::clamp(_width, MIN_SIZE, MAX_SIZE);
	height = std::clamp(_height, MIN_SIZE, MAX_SIZE);
	cells.assign(static_cast<size_t>(width) * height, Cell{});
	PlaceDefaultEndpoints();
	++revision;
}

void Grid::Resize(int _width, int _height)
{
	const int newWidth = std::clamp(_width, MIN_SIZE, MAX_SIZE);
	const int newHeight = std::clamp(_height, MIN_SIZE, MAX_SIZE);

	if (newWidth == width && newHeight == height)
	{
		return;
	}

	// copy the cells that still fit
	std::vector<Cell> resized(static_cast<size_t>(newWidth) * newHeight);

	for (int y = 0; y < std::min(height, newHeight); ++y)
	{
		for (int x = 0; x < std::min(width, newWidth); ++x)
		{
			resized[static_cast<size_t>(y) * newWidth + x] = cells[Index({ x, y })];
		}
	}

	width = newWidth;
	height = newHeight;
	cells = std::move(resized);

	// reset endpoints that are now off the grid
	if (!InBounds(start) || !InBounds(end))
	{
		PlaceDefaultEndpoints();
	}

	cells[Index(start)].walkable = true;
	cells[Index(end)].walkable = true;
	++revision;
}

void Grid::ClearWalls()
{
	for (Cell& cell : cells)
	{
		cell.walkable = true;
	}

	++revision;
}

void Grid::ResetCosts()
{
	for (Cell& cell : cells)
	{
		cell.cost = MIN_COST;
	}

	++revision;
}

void Grid::RestoreFrom(const Grid& _savedGrid)
{
	// keep the revision going up so old searches stay out of date
	const std::uint64_t nextRevision = std::max(revision, _savedGrid.revision) + 1;
	*this = _savedGrid;
	revision = nextRevision;
}

