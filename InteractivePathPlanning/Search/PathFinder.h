#pragma once

#include "Map/Grid.h"

#include <cstdint>
#include <limits>
#include <vector>

// Heuristic options, same order as the Heuristic dropdown
enum class HeuristicType
{
	Octile,
	Euclidean,
	Manhattan,
	Chebyshev,
};

// Search algorithms, same order as the Algorithm dropdown
enum class SearchAlgorithm
{
	// given cost + weight x heuristic
	AStar,
	// A* with no heuristic, spreads out by cost
	Dijkstra,
	// Ignores costs, spreads out by number of moves
	BreadthFirst,
};

// Search options
struct SearchSettings
{
	SearchAlgorithm algorithm = SearchAlgorithm::AStar;
	// A* only, 0 acts like Dijkstra and above 1 is greedier
	float heuristicWeight = 1.0f;
	HeuristicType heuristic = HeuristicType::Octile;
	bool allowDiagonal = true;
};

// Search output, with the visit order so it can be replayed
struct SearchResult
{
	// Step value for cells that never got there
	static constexpr int NEVER = std::numeric_limits<int>::max();

	// Outcome, path cost always uses cell costs even for breadth-first
	bool found = false;
	std::vector<GridPos> path;
	float pathCost = 0.0f;

	// Grid revision the search ran on
	std::uint64_t gridRevision = 0;

	// Per cell data, indexed like Grid::Index
	std::vector<float> givenCost;      // cost from the start, infinity if never reached
	std::vector<float> heuristicCost;  // estimated cost to the end
	std::vector<int> openedStep;       // step the cell joined the frontier
	std::vector<int> closedStep;       // step the cell was visited
	std::vector<int> parent;           // cell this one was reached from, -1 if none

	// Cell visited at each step, step n is expansionOrder[n - 1]
	std::vector<int> expansionOrder;

	// Number of steps the search took
	int TotalSteps() const { return static_cast<int>(expansionOrder.size()); }
};

namespace PathFinder
{
	// Estimated cost from _from to _to
	float Heuristic(HeuristicType _type, GridPos _from, GridPos _to);

	// Searches from start to end
	SearchResult Run(const Grid& _grid, const SearchSettings& _settings);

	// Cost of walking _path, diagonal moves cost 1.41x
	float PathCost(const Grid& _grid, const std::vector<GridPos>& _path);
}
