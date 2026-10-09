#pragma once

#include "Map/Grid.h"

#include <optional>
#include <vector>

// Undo and redo for the map
// Saves a full copy of the grid before each change and puts it back on undo
class EditHistory
{
private:
	// Max undo steps kept
	static constexpr size_t MAX_STEPS = 100;

	// Grid copies from before each change, newest last
	std::vector<Grid> undoSteps;
	// Grid copies from before each undo, newest last
	std::vector<Grid> redoSteps;

	// Copy from BeginChange, kept by CommitChange
	std::optional<Grid> pendingCopy;

public:
	// Saves a copy of the grid before a change
	void BeginChange(const Grid& _grid);
	// Keeps the saved copy as an undo step
	void CommitChange();

	// Undoes the last change, returns false if there's nothing to undo
	bool Undo(Grid& _grid);
	// Redoes the last undo, returns false if there's nothing to redo
	bool Redo(Grid& _grid);

	// Clears all steps
	void Clear();
};
