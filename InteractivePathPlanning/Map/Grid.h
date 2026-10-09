#pragma once

#include <cstdint>
#include <vector>

// Cell column (x) and row (y)
struct GridPos
{
	int x = 0;
	int y = 0;

	bool operator==(const GridPos&) const = default;
};

// Data for one cell
struct Cell
{
	bool walkable = true;
	float cost = 1.0f;
};

// Map of cells with costs, a start and an end
class Grid
{
private:
	// Dimensions
	int width = 0;
	int height = 0;

	// Cells, stored row by row
	std::vector<Cell> cells;

	// Endpoints
	GridPos start;
	GridPos end;

	// Goes up on every change, used to spot out of date searches
	std::uint64_t revision = 0;

	// Start on the left, end on the right, both halfway down
	void PlaceDefaultEndpoints();

public:
	// Limits
	static constexpr int MIN_SIZE = 4;
	static constexpr int MAX_SIZE = 200;
	static constexpr float MIN_COST = 1.0f;
	static constexpr float MAX_COST = 10.0f;

	// Creates an empty grid with default endpoints
	Grid(int _width, int _height);

	// Size and lookup
	int Width() const { return width; }
	int Height() const { return height; }
	int CellCount() const { return width * height; }
	bool InBounds(GridPos _pos) const { return _pos.x >= 0 && _pos.y >= 0 && _pos.x < width && _pos.y < height; }
	int Index(GridPos _pos) const { return _pos.y * width + _pos.x; }
	GridPos PosOf(int _index) const { return { _index % width, _index / width }; }
	const Cell& At(GridPos _pos) const { return cells[Index(_pos)]; }
	const Cell& At(int _index) const { return cells[_index]; }

	// Endpoints
	GridPos Start() const { return start; }
	GridPos End() const { return end; }
	bool IsEndpoint(GridPos _pos) const { return _pos == start || _pos == end; }

	// Whether ClearWalls or ResetCosts would change anything
	bool HasWalls() const;
	bool HasRaisedCosts() const;

	// Single cell edits, return true if something changed
	bool SetWalkable(GridPos _pos, bool _walkable);
	bool SetCost(GridPos _pos, float _cost);
	bool AddCost(GridPos _pos, float _delta) { return SetCost(_pos, At(_pos).cost + _delta); }
	bool SetStart(GridPos _pos);
	bool SetEnd(GridPos _pos);
	// Moves both endpoints and clears walls under them
	bool SetEndpoints(GridPos _start, GridPos _end);

	// Clears and resizes the grid
	void Reset(int _width, int _height);
	// Resizes, keeping the cells that still fit
	void Resize(int _width, int _height);
	// Makes every cell walkable
	void ClearWalls();
	// Sets every cell back to the minimum cost
	void ResetCosts();
	// Copies a saved grid back in, for undo and redo
	void RestoreFrom(const Grid& _savedGrid);

	// Goes up whenever the grid changes
	std::uint64_t Revision() const { return revision; }
};
