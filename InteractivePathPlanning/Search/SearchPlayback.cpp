#pragma region Includes

#include "Search/SearchPlayback.h"

#include <algorithm>

#pragma endregion

void SearchPlayback::Run(const Grid& _grid, const SearchSettings& _settings, bool _playFromStart)
{
	result = std::make_shared<const SearchResult>(PathFinder::Run(_grid, _settings));
	displayStep = _playFromStart ? 0 : result->TotalSteps();
	playing = _playFromStart;
	pendingSteps = 0.0;
}

void SearchPlayback::Clear()
{
	result.reset();
	displayStep = 0;
	playing = false;
	pendingSteps = 0.0;
}

void SearchPlayback::Play()
{
	if (!result || IsFinished())
	{
		return;
	}

	playing = true;
	pendingSteps = 0.0;
}

void SearchPlayback::Pause()
{
	playing = false;
}

void SearchPlayback::StepForward(const Grid& _grid, const SearchSettings& _settings, int _steps)
{
	// new search if there's none, it's out of date or it's finished
	if (!result || !MatchesGrid(_grid) || IsFinished())
	{
		Run(_grid, _settings, true);
	}

	playing = false;
	displayStep = std::min(displayStep + _steps, result->TotalSteps());
}

void SearchPlayback::ShowStep(const Grid& _grid, const SearchSettings& _settings, int _step)
{
	if (!result || !MatchesGrid(_grid))
	{
		Run(_grid, _settings, true);
	}

	playing = false;
	displayStep = std::clamp(_step, 0, result->TotalSteps());
}

void SearchPlayback::MoveByQuarters(const Grid& _grid, const SearchSettings& _settings, int _quarters)
{
	if (!result || !MatchesGrid(_grid))
	{
		Run(_grid, _settings, true);
	}

	// snap to the 25/50/75% marks
	const int quarter = std::max(1, (result->TotalSteps() + 3) / 4);
	const int quartersDone = _quarters > 0 ? displayStep / quarter : (displayStep + quarter - 1) / quarter;
	ShowStep(_grid, _settings, (quartersDone + _quarters) * quarter);
}

bool SearchPlayback::Advance(double _seconds, double _stepsPerSecond)
{
	if (!playing || !result)
	{
		return false;
	}

	pendingSteps += _seconds * _stepsPerSecond;
	const int wholeSteps = static_cast<int>(pendingSteps);
	pendingSteps -= wholeSteps;

	const int previousStep = displayStep;
	displayStep = std::min(displayStep + wholeSteps, result->TotalSteps());

	if (IsFinished())
	{
		playing = false;
	}

	return displayStep != previousStep;
}

bool SearchPlayback::IsPaused() const
{
	return result && !playing && displayStep < result->TotalSteps();
}

bool SearchPlayback::IsFinished() const
{
	return result && displayStep >= result->TotalSteps();
}

bool SearchPlayback::MatchesGrid(const Grid& _grid) const
{
	return result && result->gridRevision == _grid.Revision();
}
