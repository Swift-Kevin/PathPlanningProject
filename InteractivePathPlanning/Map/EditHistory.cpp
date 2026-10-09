#pragma region Includes

#include "Map/EditHistory.h"

#pragma endregion

void EditHistory::BeginChange(const Grid& _grid)
{
	pendingCopy = _grid;
}

void EditHistory::CommitChange()
{
	// nothing saved since the last commit
	if (!pendingCopy)
	{
		return;
	}

	undoSteps.push_back(*pendingCopy);
	pendingCopy.reset();

	if (undoSteps.size() > MAX_STEPS)
	{
		undoSteps.erase(undoSteps.begin());
	}

	// a new change clears the redo steps
	redoSteps.clear();
}

bool EditHistory::Undo(Grid& _grid)
{
	if (undoSteps.empty())
	{
		return false;
	}

	redoSteps.push_back(_grid);
	_grid.RestoreFrom(undoSteps.back());
	undoSteps.pop_back();
	pendingCopy.reset();
	return true;
}

bool EditHistory::Redo(Grid& _grid)
{
	if (redoSteps.empty())
	{
		return false;
	}

	undoSteps.push_back(_grid);
	_grid.RestoreFrom(redoSteps.back());
	redoSteps.pop_back();
	pendingCopy.reset();
	return true;
}

void EditHistory::Clear()
{
	undoSteps.clear();
	redoSteps.clear();
	pendingCopy.reset();
}
