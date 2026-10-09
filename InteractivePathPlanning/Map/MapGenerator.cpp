#pragma region Includes

#include "Map/MapGenerator.h"

#include <chrono>
#include <cmath>
#include <random>

#pragma endregion

namespace
{
	// Map density
	constexpr int CELLS_PER_WALL = 45;
	constexpr int CELLS_PER_PATCH = 160;

	// Wall length
	constexpr int MIN_WALL_LENGTH = 3;
	constexpr int MAX_WALL_LENGTH = 12;

	// Cost patches
	constexpr int MIN_PATCH_RADIUS = 2;
	constexpr int MAX_PATCH_RADIUS = 6;
	constexpr int MIN_PATCH_COST = 3;
	constexpr int MAX_PATCH_COST = 9;

	// Random int from _min to _max, inclusive
	int RandomBetween(std::mt19937& _random, int _min, int _max)
	{
		std::uniform_int_distribution<int> range(_min, _max);
		return range(_random);
	}

	// Round cost patch, highest in the middle and fading out
	void AddTerrainPatch(Grid& _grid, std::mt19937& _random)
	{
		const GridPos center{ RandomBetween(_random, 0, _grid.Width() - 1), RandomBetween(_random, 0, _grid.Height() - 1) };
		const int radius = RandomBetween(_random, MIN_PATCH_RADIUS, MAX_PATCH_RADIUS);
		const float peakCost = static_cast<float>(RandomBetween(_random, MIN_PATCH_COST, MAX_PATCH_COST));

		for (int dy = -radius; dy <= radius; ++dy)
		{
			for (int dx = -radius; dx <= radius; ++dx)
			{
				const GridPos cell{ center.x + dx, center.y + dy };
				const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

				if (!_grid.InBounds(cell) || distance > radius)
				{
					continue;
				}

				const float cost = peakCost - (peakCost - Grid::MIN_COST) * (distance / radius);

				// overlapping patches keep the higher cost
				if (cost > _grid.At(cell).cost)
				{
					_grid.SetCost(cell, cost);
				}
			}
		}
	}

	// Straight wall going right or down from a random cell
	void AddWall(Grid& _grid, std::mt19937& _random)
	{
		const bool isHorizontal = RandomBetween(_random, 0, 1) == 0;
		const int length = RandomBetween(_random, MIN_WALL_LENGTH, MAX_WALL_LENGTH);
		GridPos cell{ RandomBetween(_random, 0, _grid.Width() - 1), RandomBetween(_random, 0, _grid.Height() - 1) };

		for (int i = 0; i < length; ++i)
		{
			// SetWalkable skips endpoints and out of bounds cells
			_grid.SetWalkable(cell, false);

			isHorizontal ? ++cell.x : ++cell.y;
		}
	}
}

void MapGenerator::Generate(Grid& _grid, int _width, int _height, std::uint32_t _seed)
{
	// same seed, same map
	std::mt19937 random(_seed);
	_grid.Reset(_width, _height);

	// place endpoints first so walls skip them
	const GridPos start{ 1, RandomBetween(random, 0, _grid.Height() - 1) };
	const GridPos end{ _grid.Width() - 2, RandomBetween(random, 0, _grid.Height() - 1) };
	_grid.SetEndpoints(start, end);

	const int patchCount = _grid.CellCount() / CELLS_PER_PATCH + 1;
	const int wallCount = _grid.CellCount() / CELLS_PER_WALL;

	for (int i = 0; i < patchCount; ++i)
	{
		AddTerrainPatch(_grid, random);
	}

	for (int i = 0; i < wallCount; ++i)
	{
		AddWall(_grid, random);
	}
}

std::uint32_t MapGenerator::SeedFromClock()
{
	const std::chrono::system_clock::duration sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
	return static_cast<std::uint32_t>(sinceEpoch.count() % (static_cast<long long>(MAX_SEED) + 1));
}
