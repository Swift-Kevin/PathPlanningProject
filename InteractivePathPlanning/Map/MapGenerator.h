#pragma once

#include "Map/Grid.h"

#include <cstdint>

// Random maps, the same seed and size always give the same map
namespace MapGenerator
{
	// Fills _grid with random walls and high cost patches
	void Generate(Grid& _grid, int _width, int _height, std::uint32_t _seed);

	// New seed from the clock
	std::uint32_t SeedFromClock();

	// Largest seed the seed box can hold
	constexpr std::uint32_t MAX_SEED = 2147483646;
}
