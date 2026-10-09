#pragma once

#include "Map/Grid.h"

#include <vector>

// Built-in map drawn as rows of characters (legend in TestMaps.cpp)
struct TestMap
{
	const char* name;
	std::vector<const char*> rows;
};

namespace TestMaps
{
	// All built-in maps in Map menu order, the first one loads on startup
	const std::vector<TestMap>& All();

	// Loads _map into _grid
	void Load(const TestMap& _map, Grid& _grid);
}
