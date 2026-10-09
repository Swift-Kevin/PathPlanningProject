#pragma region Includes

#include "Search/PathFinder.h"

#include <algorithm>
#include <cmath>
#include <queue>

#pragma endregion

namespace
{
	constexpr float SQRT2 = 1.41421356f;

	// Neighbour offsets, first four straight and last four diagonal
	constexpr int DIRECTION_X[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
	constexpr int DIRECTION_Y[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
	constexpr int FIRST_DIAGONAL = 4;

	// Open list entry
	struct OpenEntry
	{
		float finalCost;
		float heuristicCost;
		int index;
	};

	// Lowest final cost first, ties go to the one closer to the end
	struct ExpandLater
	{
		bool operator()(const OpenEntry& _lhs, const OpenEntry& _rhs) const
		{
			if (_lhs.finalCost != _rhs.finalCost)
			{
				return _lhs.finalCost > _rhs.finalCost;
			}

			return _lhs.heuristicCost > _rhs.heuristicCost;
		}
	};

	// Follows parent links back from the end to build the path
	std::vector<GridPos> TracePath(const Grid& _grid, const std::vector<int>& _parent, int _endIndex)
	{
		std::vector<GridPos> path;

		for (int index = _endIndex; index != -1; index = _parent[index])
		{
			path.push_back(_grid.PosOf(index));
		}

		std::reverse(path.begin(), path.end());
		return path;
	}
}

float PathFinder::Heuristic(HeuristicType _type, GridPos _from, GridPos _to)
{
	const float dx = static_cast<float>(std::abs(_from.x - _to.x));
	const float dy = static_cast<float>(std::abs(_from.y - _to.y));

	switch (_type)
	{
	case HeuristicType::Manhattan:
		return dx + dy;
	case HeuristicType::Euclidean:
		return std::sqrt(dx * dx + dy * dy);
	case HeuristicType::Chebyshev:
		return std::max(dx, dy);
	case HeuristicType::Octile:
	default:
		// diagonal moves for the overlap, straight moves for the rest
		return (dx + dy) + (SQRT2 - 2.0f) * std::min(dx, dy);
	}
}

float PathFinder::PathCost(const Grid& _grid, const std::vector<GridPos>& _path)
{
	float cost = 0.0f;

	for (size_t i = 1; i < _path.size(); ++i)
	{
		const bool isDiagonal = _path[i].x != _path[i - 1].x && _path[i].y != _path[i - 1].y;
		cost += (isDiagonal ? SQRT2 : 1.0f) * _grid.At(_path[i]).cost;
	}

	return cost;
}

SearchResult PathFinder::Run(const Grid& _grid, const SearchSettings& _settings)
{
	// only A* uses the heuristic
	const bool isAStar = _settings.algorithm == SearchAlgorithm::AStar;
	const bool isBreadthFirst = _settings.algorithm == SearchAlgorithm::BreadthFirst;
	const float weight = isAStar ? std::max(0.0f, _settings.heuristicWeight) : 0.0f;

	const int cellCount = _grid.CellCount();
	const GridPos end = _grid.End();
	const int startIndex = _grid.Index(_grid.Start());
	const int endIndex = _grid.Index(end);
	const int directionCount = _settings.allowDiagonal ? 8 : FIRST_DIAGONAL;

	SearchResult result;
	result.gridRevision = _grid.Revision();
	result.givenCost.assign(cellCount, std::numeric_limits<float>::infinity());
	result.heuristicCost.assign(cellCount, 0.0f);
	result.openedStep.assign(cellCount, SearchResult::NEVER);
	result.closedStep.assign(cellCount, SearchResult::NEVER);
	result.parent.assign(cellCount, -1);

	std::priority_queue<OpenEntry, std::vector<OpenEntry>, ExpandLater> open;

	// add the start to the open list
	result.givenCost[startIndex] = 0.0f;
	result.heuristicCost[startIndex] = Heuristic(_settings.heuristic, _grid.Start(), end);
	result.openedStep[startIndex] = 0;
	open.push({ weight * result.heuristicCost[startIndex], result.heuristicCost[startIndex], startIndex });

	while (!open.empty())
	{
		const OpenEntry current = open.top();
		open.pop();

		// skip old copies of cells that were already visited
		if (result.closedStep[current.index] != SearchResult::NEVER)
		{
			continue;
		}

		result.expansionOrder.push_back(current.index);
		const int step = result.TotalSteps();
		result.closedStep[current.index] = step;

		if (current.index == endIndex)
		{
			result.found = true;
			break;
		}

		const GridPos pos = _grid.PosOf(current.index);

		for (int direction = 0; direction < directionCount; ++direction)
		{
			const GridPos next{ pos.x + DIRECTION_X[direction], pos.y + DIRECTION_Y[direction] };

			if (!_grid.InBounds(next) || !_grid.At(next).walkable)
			{
				continue;
			}

			const bool isDiagonal = direction >= FIRST_DIAGONAL;

			// no cutting corners
			if (isDiagonal && (!_grid.At(GridPos{ next.x, pos.y }).walkable || !_grid.At(GridPos{ pos.x, next.y }).walkable))
			{
				continue;
			}

			const int nextIndex = _grid.Index(next);

			if (result.closedStep[nextIndex] != SearchResult::NEVER)
			{
				continue;
			}

			// cell cost x move distance, breadth-first counts every move as 1
			const float stepDistance = isDiagonal ? SQRT2 : 1.0f;
			const float moveCost = isBreadthFirst ? 1.0f : stepDistance * _grid.At(next).cost;
			const float newGivenCost = result.givenCost[current.index] + moveCost;

			if (newGivenCost >= result.givenCost[nextIndex])
			{
				continue;
			}

			if (result.openedStep[nextIndex] == SearchResult::NEVER)
			{
				result.openedStep[nextIndex] = step;
				result.heuristicCost[nextIndex] = Heuristic(_settings.heuristic, next, end);
			}

			// push a new copy, the old one gets skipped later
			result.givenCost[nextIndex] = newGivenCost;
			result.parent[nextIndex] = current.index;
			open.push({ newGivenCost + weight * result.heuristicCost[nextIndex], result.heuristicCost[nextIndex], nextIndex });
		}
	}

	if (result.found)
	{
		result.path = TracePath(_grid, result.parent, endIndex);
		result.pathCost = PathCost(_grid, result.path);
	}

	return result;
}
